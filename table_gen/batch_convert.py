"""
CLI for Stage 1: convert binary MRT archives into the intermediate text format.

Usage:
    python batch_convert.py
    python batch_convert.py --source /path/to/mrt_root --output build/converted
    python batch_convert.py --force
"""

import argparse
import sys
from pathlib import Path

from config import CONVERTED_DIR, MRT_SOURCE_DIR
from pipeline.convert import (
    ConvertStatus,
    convert_mrt_file,
    discover_mrt_files,
    output_dir_for,
)


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert binary MRT archives to the intermediate text format."
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=Path(MRT_SOURCE_DIR) if MRT_SOURCE_DIR else None,
        help="Root directory containing MRT files (default: MRT_SOURCE_DIR from .env)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=CONVERTED_DIR,
        help=f"Output root directory (default: {CONVERTED_DIR})",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-convert even if output already exists",
    )
    return parser.parse_args()


def main() -> None:
    args = _parse_args()

    if args.source is None:
        print(
            "ERROR: --source is required (or set MRT_SOURCE_DIR in .env)",
            file=sys.stderr,
        )
        sys.exit(1)

    source = args.source.resolve()
    if not source.is_dir():
        print(f"ERROR: source directory not found: {source}", file=sys.stderr)
        sys.exit(1)

    output = args.output.resolve()

    entries = discover_mrt_files(source)
    if not entries:
        print(f"No MRT files found under {source}")
        return

    total = len(entries)
    success_count = 0
    skip_count = 0
    error_count = 0

    print(f"Found {total} MRT file(s) under {source}\n")

    for idx, entry in enumerate(entries, start=1):
        label = entry.rel_parent / entry.path.name
        print(f"[{idx}/{total}] {label}")

        result = convert_mrt_file(entry, output, force=args.force)

        match result.status:
            case ConvertStatus.SKIPPED:
                out_dir = output_dir_for(entry, output)
                print(f"  Skipped (already converted): {out_dir}")
                skip_count += 1
            case ConvertStatus.SUCCESS:
                print(
                    f"  Saved {result.route_count:,} routes, "
                    f"{result.port_count:,} ports"
                )
                success_count += 1
            case ConvertStatus.ERROR:
                print(f"  ERROR: {result.error_message}", file=sys.stderr)
                error_count += 1

    print(f"\nDone. Converted: {success_count}  Skipped: {skip_count}  Errors: {error_count}")


if __name__ == "__main__":
    main()
