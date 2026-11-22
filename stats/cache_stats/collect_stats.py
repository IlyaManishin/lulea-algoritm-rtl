"""Collect per-bank BRAM requirements for the sampled IPv4 routing tables."""

import argparse
import csv
import subprocess
from collections.abc import Iterator
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import NamedTuple

CACHE_STATS_DIR = Path(__file__).resolve().parent
ROOT = CACHE_STATS_DIR.parents[1]
RAMB18_BYTES = 2304
BRAM_BUDGET_BYTES = 90 * RAMB18_BYTES
ALIGNED_CSV = "cache_sizes.csv"
IN_BUDGET_CSV = "cache_sizes_in_budget.csv"
ALIGNED_COLUMNS = tuple(
    f"l{level}_{bank}_aligned"
    for level in (1, 2, 3)
    for bank in ("pop", "chunk", "seq")
)


class TableStats(NamedTuple):
    """One sampled table and its nine BRAM-aligned bank sizes in bytes."""

    path: Path
    route_count: int
    aligned: tuple[int, ...]


def write_jobs(sampled: Path, jobs: Path) -> list[Path]:
    """List sampled tables and write their absolute paths in cache_gen TSV format."""
    tables = sorted(sampled.glob("*/*/*/seed_*/routes.txt"))
    if not tables:
        raise ValueError(f"No sampled tables found in {sampled}")

    with jobs.open("w", encoding="utf-8") as stream:
        for table in tables:
            size = table.relative_to(sampled).parts[0]
            if not size.isascii() or not size.isdecimal() or int(size) == 0:
                raise ValueError(f"Invalid table size directory: {table}")
            if any(char in str(table) for char in "\t\r\n"):
                raise ValueError(f"Path cannot be represented in a cache_gen job: {table!r}")
            stream.write(f"stats\t{table}\n")
    return tables


def read_builder_stats(raw: Path, sampled: Path, tables: list[Path]) -> Iterator[TableStats]:
    """Read the aligned bank sizes in the same order as the jobs."""
    with raw.open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source)
        if not {"file", *ALIGNED_COLUMNS}.issubset(reader.fieldnames or ()):
            raise ValueError(f"Missing aligned cache statistics in {raw}")
        for table, row in zip(tables, reader, strict=True):
            if row["file"] != str(table):
                raise ValueError(f"Expected statistics for {table}, got {row['file']}")
            relative = table.relative_to(sampled)
            yield TableStats(
                path=relative,
                route_count=int(relative.parts[0]),
                aligned=tuple(int(row[column]) for column in ALIGNED_COLUMNS),
            )


def write_cache_csvs(output_dir: Path, stats: Iterator[TableStats]) -> int:
    """Write aligned and budget-filtered sizes. Return the number that fit."""
    in_budget = 0
    with (
        (output_dir / ALIGNED_CSV).open("w", encoding="utf-8", newline="") as aligned_file,
        (output_dir / IN_BUDGET_CSV).open("w", encoding="utf-8", newline="") as in_budget_file,
    ):
        aligned = csv.writer(aligned_file)
        filtered = csv.writer(in_budget_file)
        aligned.writerow(("file", "table_size", *ALIGNED_COLUMNS))
        filtered.writerow(("file", "table_size", *ALIGNED_COLUMNS))

        for table in stats:
            metadata = (table.path, table.route_count)
            aligned.writerow((*metadata, *table.aligned))
            if sum(table.aligned) <= BRAM_BUDGET_BYTES:
                filtered.writerow((*metadata, *table.aligned))
                in_budget += 1
    return in_budget


def collect(sampled: Path, builder: Path, output_dir: Path) -> None:
    """Run the builder once and publish the job list and two CSVs."""
    sampled = sampled.resolve(strict=True)
    builder = builder.resolve(strict=True)
    output_dir.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=output_dir) as tmp:
        work = Path(tmp)
        jobs = work / "jobs.tsv"
        tables = write_jobs(sampled, jobs)
        print(f"Collecting statistics for {len(tables)} tables from {sampled}", flush=True)
        raw = work / "builder.csv"
        subprocess.run([builder, jobs, "--out-csv", raw], check=True)
        in_budget = write_cache_csvs(work, read_builder_stats(raw, sampled, tables))
        for name in (jobs.name, ALIGNED_CSV, IN_BUDGET_CSV):
            (work / name).replace(output_dir / name)
    print(f"Wrote statistics for {len(tables)} tables to {output_dir}")
    print(f"{in_budget} tables fit the BRAM budget of {BRAM_BUDGET_BYTES} bytes")


def main() -> None:
    """Run the collector with repository defaults or explicit paths."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sampled", type=Path, default=ROOT / "build/sampled")
    parser.add_argument("--builder", type=Path, default=ROOT / "cache_gen/build/lulea_builder")
    parser.add_argument("--output-dir", type=Path, default=CACHE_STATS_DIR / "build")
    args = parser.parse_args()
    try:
        collect(args.sampled, args.builder, args.output_dir)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")


if __name__ == "__main__":
    main()
