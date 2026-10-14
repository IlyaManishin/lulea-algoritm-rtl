"""
Stage 2 logic: plan and execute sampling of converted full-tables into
fixed-size sub-tables.

This module contains no CLI or presentation code.
"""

import math
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path

from config import PORT_MAP_FILENAME, ROUTE_FILENAME
from core.gen_types import RouteRecord
from core.mrt_gen import filter_mrt_routes
from utils.port_mapping import map_next_hops_to_ports
from utils.route_reader import RouteReadError, read_route_count, read_routes
from utils.route_saver import save_converted


# =========================================================================
# Data types
# =========================================================================

@dataclass(frozen=True, slots=True)
class SampleTask:
    """One unit of work: sample a converted table at a given size and seed."""
    source_path: Path
    output_dir: Path
    target_size: int
    seed: int
    source_label: str   # short display name, e.g. "rib.20241001.0000"


class SampleStatus(Enum):
    SUCCESS = auto()
    SKIPPED = auto()
    ERROR = auto()


@dataclass(frozen=True, slots=True)
class SampleResult:
    """Outcome of executing a single SampleTask."""
    task: SampleTask
    status: SampleStatus
    error_message: str = ""


@dataclass(frozen=True)
class SamplePlan:
    """Complete work plan for batch sampling."""
    tasks: list[SampleTask] = field(default_factory=list)
    source_count: int = 0

    @property
    def total_tasks(self) -> int:
        return len(self.tasks)


# =========================================================================
# Helpers
# =========================================================================

def compute_sample_count(
    total_routes: int,
    target_size: int,
    coeff: float,
) -> int:
    """
    Number of samples to generate.

        n = floor(total_routes * coeff / target_size)

    Returns 0 if the table is too small for even one sample.
    """
    if total_routes < target_size:
        return 0
    return math.floor(total_routes * coeff / target_size)


def _discover_converted_tables(converted_root: Path) -> list[Path]:
    """Return sorted paths of all routes.txt files under converted_root."""
    return sorted(converted_root.rglob(ROUTE_FILENAME))


def _sample_output_dir(
    output_root: Path,
    converted_root: Path,
    routes_file: Path,
    target_size: int,
    seed: int,
) -> Path:
    """Build: output_root / size / <rel_parent> / <mrt_stem> / seed_N."""
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


# =========================================================================
# Plan builder
# =========================================================================

def build_sample_plan(
    converted_root: Path,
    output_root: Path,
    sizes: list[int],
    coeff: float,
) -> SamplePlan:
    """
    Scan converted tables and build the full work plan.

    Uses count headers for O(1) capacity checks (no route loading).
    Tasks are grouped by source file for cache-friendly execution.
    """
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


# =========================================================================
# Executor
# =========================================================================

class SampleExecutor:
    """
    Executes SampleTasks with a lazy single-entry route cache.

    Keeps only the last loaded source in memory (~1M records ≈ 100 MB).
    Expects tasks grouped by source_path for optimal cache utilisation.
    """

    def __init__(self) -> None:
        self._cached_path: Path | None = None
        self._cached_routes: list[RouteRecord] = []

    def execute(
        self,
        task: SampleTask,
        *,
        force: bool = False,
    ) -> SampleResult:
        if not force and _is_already_sampled(task.output_dir):
            return SampleResult(task=task, status=SampleStatus.SKIPPED)

        try:
            routes = self._load_routes(task.source_path)
        except (FileNotFoundError, RouteReadError) as exc:
            return SampleResult(
                task=task,
                status=SampleStatus.ERROR,
                error_message=str(exc),
            )

        sampled = filter_mrt_routes(routes, target_count=task.target_size, seed=task.seed)
        if not sampled:
            return SampleResult(
                task=task,
                status=SampleStatus.ERROR,
                error_message="filter_mrt_routes returned empty result",
            )

        port_map = map_next_hops_to_ports(sampled)

        try:
            save_converted(
                routes=sampled,
                port_map=port_map,
                routes_path=task.output_dir / ROUTE_FILENAME,
                port_map_path=task.output_dir / PORT_MAP_FILENAME,
            )
        except OSError as exc:
            return SampleResult(
                task=task,
                status=SampleStatus.ERROR,
                error_message=str(exc),
            )

        return SampleResult(task=task, status=SampleStatus.SUCCESS)

    def _load_routes(self, source_path: Path) -> list[RouteRecord]:
        if self._cached_path != source_path:
            self._cached_routes = read_routes(source_path)
            self._cached_path = source_path
        return self._cached_routes
