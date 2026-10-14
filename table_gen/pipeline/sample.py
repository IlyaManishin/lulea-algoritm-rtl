"""Stage 2: plan and execute sampling of converted full-tables into fixed-size sub-tables."""

import math
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path

from config import PORT_MAP_FILENAME, ROUTE_FILENAME
from core.gen_types import RouteRecord
from core.mrt_gen import distribution_filter_mrt_routes
from utils.route_reader import RouteReadError, read_port_map, read_route_count, read_routes
from utils.route_saver import FileFormat, save_converted


@dataclass(frozen=True, slots=True)
class SampleTask:
    source_path: Path
    output_dir: Path
    target_size: int
    seed: int
    source_label: str


class SampleStatus(Enum):
    SUCCESS = auto()
    SKIPPED = auto()
    ERROR = auto()


@dataclass(frozen=True, slots=True)
class SampleResult:
    task: SampleTask
    status: SampleStatus
    error_message: str = ""


@dataclass(frozen=True)
class SamplePlan:
    tasks: list[SampleTask] = field(default_factory=list)
    source_count: int = 0

    @property
    def total_tasks(self) -> int:
        return len(self.tasks)


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
    """Build complete work plan using O(1) header counts."""
    routes_files = _discover_converted_tables(converted_root)
    if not routes_files:
        return SamplePlan()

    tasks: list[SampleTask] = []

    for routes_file in routes_files:
        try:
            total = read_route_count(routes_file)
        except (FileNotFoundError, RouteReadError):
            continue

        source_label = routes_file.parent.name

        for size in sizes:
            n = compute_sample_count(total, size, coeff)
            for seed in range(n):
                out_dir = _sample_output_dir(
                    output_root, converted_root, routes_file, size, seed,
                )
                tasks.append(SampleTask(
                    source_path=routes_file,
                    output_dir=out_dir,
                    target_size=size,
                    seed=seed,
                    source_label=source_label,
                ))

    return SamplePlan(tasks=tasks, source_count=len(routes_files))


class SampleExecutor:
    """Executes SampleTasks with a lazy single-entry route and port cache."""

    def __init__(self) -> None:
        self._cached_path: Path | None = None
        self._cached_routes: list[RouteRecord] = []
        self._cached_port_map: dict[int, str] = {}

    def execute(
        self,
        task: SampleTask,
        *,
        force: bool = False,
    ) -> SampleResult:
        if not force and _is_already_sampled(task.output_dir):
            return SampleResult(task=task, status=SampleStatus.SKIPPED)

        try:
            routes: list[RouteRecord]
            port_map: dict[int, str]
            routes, port_map = self._load_data(task.source_path)
        except (FileNotFoundError, RouteReadError) as exc:
            return SampleResult(
                task=task,
                status=SampleStatus.ERROR,
                error_message=str(exc),
            )

        sampled: list[RouteRecord] = distribution_filter_mrt_routes(routes, target_count=task.target_size, seed=task.seed)
        if not sampled:
            return SampleResult(
                task=task,
                status=SampleStatus.ERROR,
                error_message="filter_mrt_routes returned empty result",
            )

        used_ports = {r.port_id for r in sampled}
        sample_port_map: dict[int, str] = {pid: port_map[pid] for pid in used_ports if pid in port_map}

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

    def _load_data(self, source_path: Path) -> tuple[list[RouteRecord], dict[int, str]]:
        if self._cached_path != source_path:
            self._cached_routes = read_routes(source_path)
            port_path = source_path.parent / PORT_MAP_FILENAME
            self._cached_port_map = read_port_map(port_path) if port_path.is_file() else {}
            self._cached_path = source_path
        return self._cached_routes, self._cached_port_map
