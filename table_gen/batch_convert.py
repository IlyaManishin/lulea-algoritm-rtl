"""
Stage 1 of the batch pipeline: convert binary MRT archives into the
lightweight plain-text intermediate format.

Usage (from table_gen/):
    python batch_convert.py
    python batch_convert.py --source /path/to/mrt_root --output build/converted

Input layout  ($MRT_SOURCE_DIR):
    <source>/
        <file>.bz2          # RouteViews RIB dumps
        <file>.gz           # RIPE RIS bview dumps
        <file>              # already-decompressed binary MRT

Output layout (CONVERTED_DIR):
    build/converted/
        <source>/
            <stem>/          # stem = filename without archive extension(s)
                routes.txt   # full unique-prefix table (count header + prefix next_hop)
                routes.ports # next_hop→port mapping (count header + next_hop port_id)

Each MRT file is skipped if its output directory already exists and both output
files are non-empty (idempotent re-runs).
"""

import argparse
import bz2
import gzip
import shutil
import sys
import tempfile
from pathlib import Path

from config import (
    CONVERTED_DIR,
    MRT_SOURCE_DIR,
    PORT_MAP_FILENAME,
    ROUTE_FILENAME,
)
from core.mrt_reader import MRTParseError, parse_mrt_binary
from core.route_io import write_routes_and_port_map
from utils.port_mapping import map_next_hops_to_ports


# =========================================================================
# MRT discovery
# =========================================================================

# Archive suffixes recognised by this script, in the order they are stripped.
_ARCHIVE_SUFFIXES = (".bz2", ".gz")


def _stem(path: Path) -> str:
    """Return filename with all recognised archive suffixes stripped."""
    name = path.name
    for suffix in _ARCHIVE_SUFFIXES:
        if name.endswith(suffix):
            name = name[: -len(suffix)]
            break  # at most one archive wrapper
    return name


def _discover_mrt_files(source_root: Path) -> list[tuple[Path, Path]]:
    """
    Walk source_root and return (mrt_file, relative_parent) pairs.

    relative_parent is the path of the containing directory relative to
    source_root (e.g. Path("routeviews")).
    """
    found: list[tuple[Path, Path]] = []
    for candidate in sorted(source_root.rglob("*")):
        if not candidate.is_file():
            continue
        rel_parent = candidate.parent.relative_to(source_root)
        found.append((candidate, rel_parent))
    return found


# =========================================================================
# Decompression helpers
# =========================================================================

def _decompress_to_tmp(src: Path) -> Path:
    """
    Decompress a .bz2 or .gz file into a named temporary file.
    The caller is responsible for deleting the returned path.
    """
    suffix = src.suffix.lower()
    tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".mrt")
    tmp_path = Path(tmp.name)
    tmp.close()

    if suffix == ".bz2":
        with bz2.open(src, "rb") as f_in, open(tmp_path, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)
    elif suffix == ".gz":
        with gzip.open(src, "rb") as f_in, open(tmp_path, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)
    else:
        raise ValueError(f"Unsupported archive format: {src.suffix}")

    return tmp_path


# =========================================================================
# Single-file conversion
# =========================================================================

def _output_dir(output_root: Path, rel_parent: Path, stem: str) -> Path:
    return output_root / rel_parent / stem


def _already_converted(out_dir: Path) -> bool:
    routes_file = out_dir / ROUTE_FILENAME
    ports_file = out_dir / PORT_MAP_FILENAME
    return (
        out_dir.exists()
        and routes_file.exists()
        and routes_file.stat().st_size > 0
        and ports_file.exists()
        and ports_file.stat().st_size > 0
    )


def convert_one(
    mrt_path: Path,
    out_dir: Path,
    *,
    verbose: bool = True,
) -> bool:
    """
    Parse a single MRT file and write converted output files.

    Returns True on success, False on parse error.
    Decompresses .bz2 / .gz into a temp file; the temp file is cleaned up
    regardless of success or failure.
    """
    suffix = mrt_path.suffix.lower()
    tmp_path: Path | None = None

    try:
        if suffix in (".bz2", ".gz"):
            if verbose:
                print(f"  Decompressing {mrt_path.name} ...", flush=True)
            tmp_path = _decompress_to_tmp(mrt_path)
            parse_target = tmp_path
        else:
            parse_target = mrt_path

        if verbose:
            print(f"  Parsing binary MRT ({parse_target}) ...", flush=True)

        routes = parse_mrt_binary(parse_target)

    except (OSError, MRTParseError, ValueError) as exc:
        print(f"  ERROR: {exc}", file=sys.stderr)
        return False
    finally:
        if tmp_path is not None and tmp_path.exists():
            tmp_path.unlink()

    if not routes:
        print(f"  WARNING: no routes found in {mrt_path.name}, skipping.")
        return False

    port_map = map_next_hops_to_ports(routes)

    routes_path = out_dir / ROUTE_FILENAME
    ports_path = out_dir / PORT_MAP_FILENAME

    write_routes_and_port_map(
        routes=routes,
        port_map=port_map,
        routes_path=routes_path,
        port_map_path=ports_path,
    )

    if verbose:
        print(
            f"  Saved {len(routes):,} routes, {len(port_map):,} ports → {out_dir}"
        )

    return True


# =========================================================================
# Batch runner
# =========================================================================

def run_batch_convert(
    source_root: Path,
    output_root: Path,
    *,
    force: bool = False,
) -> None:
    files = _discover_mrt_files(source_root)
    if not files:
        print(f"No MRT files found under {source_root}")
        return

    total = len(files)
    converted = 0
    skipped = 0
    errors = 0

    print(f"Found {total} MRT file(s) under {source_root}\n")

    for idx, (mrt_path, rel_parent) in enumerate(files, start=1):
        stem = _stem(mrt_path)
        out_dir = _output_dir(output_root, rel_parent, stem)

        print(f"[{idx}/{total}] {rel_parent / mrt_path.name}")

        if not force and _already_converted(out_dir):
            print(f"  Skipped (already converted): {out_dir}")
            skipped += 1
            continue

        ok = convert_one(mrt_path, out_dir, verbose=True)
        if ok:
            converted += 1
        else:
            errors += 1

        print()

    print("=" * 60)
    print(f"Done. Converted: {converted}  Skipped: {skipped}  Errors: {errors}")


# =========================================================================
# CLI
# =========================================================================

def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Convert binary MRT archives to the plain-text intermediate format "
            "for subsequent batch sampling."
        )
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=Path(MRT_SOURCE_DIR) if MRT_SOURCE_DIR else None,
        help="Root directory containing MRT files / subdirs (default: MRT_SOURCE_DIR from .env)",
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

    run_batch_convert(source, args.output.resolve(), force=args.force)


if __name__ == "__main__":
    main()
