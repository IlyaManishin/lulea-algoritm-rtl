"""
Stage 2 of the batch pipeline: sample converted full-tables into
multiple fixed-size sub-tables.

Usage (from table_gen/):
    python batch_sample.py
    python batch_sample.py --input build/converted --output build/sampled
    python batch_sample.py --sizes 1024 4096 --coeff 2.0

How many samples per (source, size)?
    n_samples = floor(total_routes * coeff / target_size)

    With coeff=1.0: exactly as many samples as fit without reusing routes.
    With coeff=2.0: routes are reused ~2x on average.

Output layout:
    build/sampled/
        <size>/
            <source>/
                <mrt_stem>/
                    seed_0/
                        routes.txt
                        routes.ports
                    seed_1/
                        ...
"""

import argparse
import math
import sys
from pathlib import Path

from rich.progress import (
    BarColumn,
    MofNCompleteColumn,
    Progress,
    SpinnerColumn,
    TextColumn,
    TimeElapsedColumn,
    TimeRemainingColumn,
)

from config import (
    CONVERTED_DIR,
    PORT_MAP_FILENAME,
    ROUTE_FILENAME,
    SAMPLE_FILL_COEFF,
    SAMPLED_DIR,
    TABLE_SIZES,
)
from core.mrt_gen import filter_mrt_routes
from core.route_io import RouteIOError, read_route_count, read_routes, write_routes_and_port_map
from utils.port_mapping import map_next_hops_to_ports


# =========================================================================
# Discovery of converted tables
# =========================================================================

def _discover_converted_tables(converted_root: Path) -> list[Path]:
    """
    Return all routes.txt files found under converted_root.
    Each represents one fully-parsed source MRT table.
    """
    return sorted(converted_root.rglob(ROUTE_FILENAME))


# =========================================================================
# Sample count calculation
# =========================================================================

def n_samples_for(total_routes: int, target_size: int, coeff: float) -> int:
    """
    Number of samples to generate from a table of total_routes routes.

        n = floor(total_routes * coeff / target_size)

    Returns 0 if the table is too small to produce even one sample.
    """
    if total_routes < target_size:
        return 0
    return math.floor(total_routes * coeff / target_size)


# =========================================================================
# Output path helpers
# =========================================================================

def _sample_dir(
    output_root: Path,
    converted_root: Path,
    routes_file: Path,
    target_size: int,
    seed_idx: int,
) -> Path:
    """
    Build the output directory for one sample.

    Mirrors the structure of the converted table but adds the size at the top
    and a seed sub-directory at the bottom:
        <output_root>/<size>/<source>/<mrt_stem>/seed_<N>/
    """
    # rel_path is e.g. routeviews/rib.20241001.0000/routes.txt
    rel_routes = routes_file.relative_to(converted_root)
    # Drop the filename; the parent is e.g. routeviews/rib.20241001.0000
    rel_stem_dir = rel_routes.parent
    return output_root / str(target_size) / rel_stem_dir / f"seed_{seed_idx}"


def _already_sampled(out_dir: Path) -> bool:
    routes_file = out_dir / ROUTE_FILENAME
    ports_file = out_dir / PORT_MAP_FILENAME
    return (
        out_dir.exists()
        and routes_file.exists()
        and routes_file.stat().st_size > 0
        and ports_file.exists()
        and ports_file.stat().st_size > 0
    )


# =========================================================================
# Batch sampler
# =========================================================================

def run_batch_sample(
    converted_root: Path,
    output_root: Path,
    table_sizes: list[int],
    coeff: float,
    *,
    force: bool = False,
) -> None:
    routes_files = _discover_converted_tables(converted_root)

    if not routes_files:
        print(f"No converted tables found under {converted_root}")
        print("Run batch_convert.py first.")
        return

    print(f"Found {len(routes_files)} converted table(s).\n")
    print(f"Target sizes  : {table_sizes}")
    print(f"Fill coeff    : {coeff}")
    print()

    # -------------------------------------------------------------------------
    # Pre-scan: read count headers to build the full work plan without loading
    # any routes yet.  This lets the progress bar show total work upfront.
    # -------------------------------------------------------------------------
    WorkItem = tuple[Path, int, int, int]  # (routes_file, target_size, seed_idx, total_routes)
    work_items: list[WorkItem] = []

    for routes_file in routes_files:
        try:
            total = read_route_count(routes_file)
        except (FileNotFoundError, RouteIOError) as exc:
            print(f"WARNING: skipping {routes_file}: {exc}", file=sys.stderr)
            continue

        for size in table_sizes:
            n = n_samples_for(total, size, coeff)
            for seed_idx in range(n):
                work_items.append((routes_file, size, seed_idx, total))

    if not work_items:
        print("No work to do (all tables too small for the requested sizes?).")
        return

    skipped_count = 0
    error_count = 0

    # Cache loaded routes keyed by routes_file path to avoid reloading the
    # same table for every (size, seed) combination.
    _route_cache: dict[Path, list] = {}

    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        BarColumn(),
        MofNCompleteColumn(),
        TimeElapsedColumn(),
        TimeRemainingColumn(),
        refresh_per_second=8,
    ) as progress:
        task = progress.add_task("Sampling ...", total=len(work_items))

        for routes_file, target_size, seed_idx, total_routes in work_items:
            out_dir = _sample_dir(output_root, converted_root, routes_file, target_size, seed_idx)

            progress.update(
                task,
                description=(
                    f"[cyan]{routes_file.parent.name}[/cyan] "
                    f"size={target_size} seed={seed_idx}"
                ),
            )

            if not force and _already_sampled(out_dir):
                skipped_count += 1
                progress.advance(task)
                continue

            # Load routes lazily (once per source file)
            if routes_file not in _route_cache:
                try:
                    _route_cache[routes_file] = read_routes(routes_file)
                except (FileNotFoundError, RouteIOError) as exc:
                    progress.print(f"[red]ERROR loading {routes_file}: {exc}[/red]")
                    error_count += 1
                    progress.advance(task)
                    continue

            all_routes = _route_cache[routes_file]

            sampled = filter_mrt_routes(all_routes, target_count=target_size, seed=seed_idx)
            if not sampled:
                error_count += 1
                progress.advance(task)
                continue

            port_map = map_next_hops_to_ports(sampled)

            try:
                write_routes_and_port_map(
                    routes=sampled,
                    port_map=port_map,
                    routes_path=out_dir / ROUTE_FILENAME,
                    port_map_path=out_dir / PORT_MAP_FILENAME,
                )
            except OSError as exc:
                progress.print(f"[red]ERROR writing {out_dir}: {exc}[/red]")
                error_count += 1
                progress.advance(task)
                continue

            progress.advance(task)

    done = len(work_items) - skipped_count - error_count
    print(f"\nDone. Generated: {done}  Skipped: {skipped_count}  Errors: {error_count}")


# =========================================================================
# CLI
# =========================================================================

def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Sample converted full-tables into fixed-size sub-tables "
            "for Luleå algorithm statistics collection."
        )
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=CONVERTED_DIR,
        help=f"Root of converted tables (default: {CONVERTED_DIR})",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=SAMPLED_DIR,
        help=f"Output root directory (default: {SAMPLED_DIR})",
    )
    parser.add_argument(
        "--sizes",
        type=int,
        nargs="+",
        default=TABLE_SIZES,
        metavar="N",
        help=f"Target route-table sizes to generate (default: {TABLE_SIZES})",
    )
    parser.add_argument(
        "--coeff",
        type=float,
        default=SAMPLE_FILL_COEFF,
        metavar="C",
        help=(
            "Fill coefficient: n_samples = floor(total * C / size). "
            f"Default: {SAMPLE_FILL_COEFF}"
        ),
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-generate even if output already exists",
    )
    return parser.parse_args()


def main() -> None:
    args = _parse_args()

    converted = args.input.resolve()
    if not converted.is_dir():
        print(f"ERROR: converted input directory not found: {converted}", file=sys.stderr)
        sys.exit(1)

    if args.coeff <= 0:
        print("ERROR: --coeff must be a positive number", file=sys.stderr)
        sys.exit(1)

    run_batch_sample(
        converted_root=converted,
        output_root=args.output.resolve(),
        table_sizes=sorted(set(args.sizes)),
        coeff=args.coeff,
        force=args.force,
    )


if __name__ == "__main__":
    main()
