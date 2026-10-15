"""Stage 2 CLI: sample converted full-tables into fixed-size sub-tables."""

import argparse
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

from engine.config import CONVERTED_DIR, SAMPLE_FILL_COEFF, SAMPLED_DIR, TABLE_SIZES
from engine.pipeline.sample import (
    GroupLoadError,
    SampleExecutor,
    SamplePlan,
    SampleStatus,
    build_sample_plan,
)


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Sample converted full-tables into fixed-size sub-tables."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=CONVERTED_DIR,
        help=f"Root directory of converted tables (default: {CONVERTED_DIR})",
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
        help=f"Sample fill coefficient (default: {SAMPLE_FILL_COEFF})",
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
        print(
            f"ERROR: converted input directory not found: {converted}", file=sys.stderr)
        sys.exit(1)

    if args.coeff <= 0:
        print("ERROR: --coeff must be a positive number", file=sys.stderr)
        sys.exit(1)

    output = args.output.resolve()
    target_sizes = sorted(set(args.sizes))

    plan: SamplePlan = build_sample_plan(
        converted_root=converted,
        output_root=output,
        sizes=target_sizes,
        coeff=args.coeff,
    )

    if plan.total_tasks == 0:
        print(
            f"No converted tables found under {converted} or all tables too small.")
        print("Run batch_convert.py first.")
        return

    print(f"Found {plan.source_count} converted table(s).")
    print(f"Target sizes : {target_sizes}")
    print(f"Fill coeff   : {args.coeff}")
    print(f"Total tasks  : {plan.total_tasks}\n")

    success_count = 0
    skip_count = 0
    error_count = 0

    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        BarColumn(),
        MofNCompleteColumn(),
        TimeElapsedColumn(),
        TimeRemainingColumn(),
        refresh_per_second=8,
    ) as progress:
        progress_task = progress.add_task(
            "Sampling ...", total=plan.total_tasks)

        for group in plan.groups:
            progress.update(
                progress_task,
                description=f"[cyan]{group.source_label}[/cyan]",
            )

            try:
                executor = SampleExecutor(group)
            except GroupLoadError as exc:
                progress.print(
                    f"[red]ERROR loading {group.source_label}: {exc}[/red]")
                error_count += len(group.tasks)
                progress.advance(progress_task, advance=len(group.tasks))
                continue

            for result in executor.execute_all(force=args.force):
                task = result.task
                progress.update(
                    progress_task,
                    description=(
                        f"[cyan]{group.source_label}[/cyan] "
                        f"size={task.target_size} seed={task.seed}"
                    ),
                )

                match result.status:
                    case SampleStatus.SUCCESS:
                        success_count += 1
                    case SampleStatus.SKIPPED:
                        skip_count += 1
                    case SampleStatus.ERROR:
                        progress.print(
                            f"[red]ERROR {group.source_label} "
                            f"(size={task.target_size}, seed={task.seed}): "
                            f"{result.error_message}[/red]"
                        )
                        error_count += 1

                progress.advance(progress_task)

    print(
        f"\nDone. Generated: {success_count}  "
        f"Skipped: {skip_count}  "
        f"Errors: {error_count}"
    )


if __name__ == "__main__":
    main()
