from collections.abc import Iterator
import math
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path

from config import PORT_MAP_FILENAME, ROUTE_FILENAME, SAMPLE_WORKERS
from core.gen_types import PortMapping, RouteRecord
from core.mrt_filter import filter_mrt_routes
from utils.concurrency import run_in_fork_process_pool
from utils.route_reader import RouteReadError, read_port_map, read_route_count, read_routes
from utils.route_saver import FileFormat, save_converted


@dataclass(frozen=True, slots=True)
class SingleSampleTask:
    output_dir: Path
    target_size: int
    seed: int


@dataclass(frozen=True, slots=True)
class FileSampleGroup:
    source_path: Path
    source_label: str
    total_routes: int
    tasks: list[SingleSampleTask]


@dataclass(frozen=True)
class SamplePlan:
    groups: list[FileSampleGroup] = field(default_factory=list)

    @property
    def total_tasks(self) -> int:
        return sum(len(group.tasks) for group in self.groups)

    @property
    def source_count(self) -> int:
        return len(self.groups)


class SampleStatus(Enum):
    SUCCESS = auto()
    SKIPPED = auto()
    ERROR = auto()


class GroupLoadError(Exception):
    """Raised when group source data fails to load upfront."""
    pass


@dataclass(frozen=True, slots=True)
class SampleResult:
    task: SingleSampleTask
    status: SampleStatus
    error_message: str = ""


def compute_sample_count(
    total_routes: int,
    target_size: int,
    coeff: float,
) -> int:
    """Calculate sample count: floor(total_routes * coeff / target_size)."""
    if total_routes < target_size:
        return 0
    return math.floor(total_routes * coeff / target_size)


def _discover_converted_tables(converted_root: Path) -> list[Path]:
    return sorted(converted_root.rglob(ROUTE_FILENAME))


def _sample_output_dir(
    output_root: Path,
    converted_root: Path,
    routes_file: Path,
    target_size: int,
    seed: int,
) -> Path:
    rel_dir = routes_file.parent.relative_to(converted_root)
    return output_root / str(target_size) / rel_dir / f"seed_{seed}"


def _is_already_sampled(out_dir: Path) -> bool:
    routes_file = out_dir / ROUTE_FILENAME
    ports_file = out_dir / PORT_MAP_FILENAME
    return (
        routes_file.exists()
        and routes_file.stat().st_size > 0
        and ports_file.exists()
        and ports_file.stat().st_size > 0
    )


def build_sample_plan(
    converted_root: Path,
    output_root: Path,
    sizes: list[int],
    coeff: float,
) -> SamplePlan:
    """Build work plan grouped by converted table file using O(1) header counts."""
    routes_files = _discover_converted_tables(converted_root)
    if not routes_files:
        return SamplePlan()

    groups: list[FileSampleGroup] = []

    for routes_file in routes_files:
        try:
            total = read_route_count(routes_file)
        except (FileNotFoundError, RouteReadError):
            continue

        source_label = routes_file.parent.name
        tasks: list[SingleSampleTask] = []

        for size in sizes:
            n = compute_sample_count(total, size, coeff)
            for seed in range(n):
                out_dir = _sample_output_dir(
                    output_root, converted_root, routes_file, size, seed,
                )
                tasks.append(SingleSampleTask(
                    output_dir=out_dir,
                    target_size=size,
                    seed=seed,
                ))

        if tasks:
            groups.append(FileSampleGroup(
                source_path=routes_file,
                source_label=source_label,
                total_routes=total,
                tasks=tasks,
            ))

    return SamplePlan(groups=groups)


class SampleExecutor:
    """Executes sampling for a single FileSampleGroup with fail-fast loading."""

    def __init__(self, group: FileSampleGroup) -> None:
        self._group: FileSampleGroup = group
        try:
            self._routes: list[RouteRecord] = read_routes(group.source_path)
            port_path = group.source_path.parent / PORT_MAP_FILENAME
            self._port_map: PortMapping = read_port_map(
                port_path) if port_path.is_file() else {}
        except (FileNotFoundError, RouteReadError, OSError) as exc:
            raise GroupLoadError(
                f"Failed to load source table '{group.source_path}': {exc}"
            ) from exc

_G_ROUTES: list[RouteRecord] = []
_G_PORT_MAP: PortMapping = {}


def _init_sample_worker(routes: list[RouteRecord], port_map: PortMapping) -> None:
    global _G_ROUTES, _G_PORT_MAP
    _G_ROUTES = routes
    _G_PORT_MAP = port_map


def _execute_sample_worker_task(args: tuple[SingleSampleTask, bool]) -> SampleResult:
    task, force = args
    if not force and _is_already_sampled(task.output_dir):
        return SampleResult(task=task, status=SampleStatus.SKIPPED)

    sampled: list[RouteRecord] = filter_mrt_routes(
        _G_ROUTES, target_count=task.target_size, seed=task.seed
    )
    if not sampled:
        return SampleResult(
            task=task,
            status=SampleStatus.ERROR,
            error_message="filter_mrt_routes returned empty result",
        )

    used_ports = {r.port_id for r in sampled}
    sample_port_map: PortMapping = {
        pid: _G_PORT_MAP[pid] for pid in used_ports if pid in _G_PORT_MAP
    }

    try:
        save_converted(
            routes=sampled,
            port_map=sample_port_map,
            routes_path=task.output_dir / ROUTE_FILENAME,
            port_map_path=task.output_dir / PORT_MAP_FILENAME,
            file_format=FileFormat.BINARY,
        )
    except OSError as exc:
        return SampleResult(
            task=task,
            status=SampleStatus.ERROR,
            error_message=str(exc),
        )

    return SampleResult(task=task, status=SampleStatus.SUCCESS)


class SampleExecutor:
    """Executes sampling for a single FileSampleGroup with fail-fast loading."""

    def __init__(self, group: FileSampleGroup) -> None:
        self._group: FileSampleGroup = group
        try:
            self._routes: list[RouteRecord] = read_routes(group.source_path)
            port_path = group.source_path.parent / PORT_MAP_FILENAME
            self._port_map: PortMapping = read_port_map(
                port_path) if port_path.is_file() else {}
        except (FileNotFoundError, RouteReadError, OSError) as exc:
            raise GroupLoadError(
                f"Failed to load source table '{group.source_path}': {exc}"
            ) from exc

    def execute_all(
        self,
        *,
        force: bool = False,
        max_workers: int = SAMPLE_WORKERS,
    ) -> Iterator[SampleResult]:
        """Iterate and execute all sample tasks in the group across processes."""
        task_items = [(task, force) for task in self._group.tasks]
        yield from run_in_fork_process_pool(
            _execute_sample_worker_task,
            task_items,
            max_workers=max_workers,
            initializer=_init_sample_worker,
            initargs=(self._routes, self._port_map),
        )
