"""Place two spare RAMB18 blocks to cover the most 9,728-route tables."""

import csv

from collect_stats import ALIGNED_COLUMNS, ALIGNED_CSV, CACHE_STATS_DIR, RAMB18_BYTES

# Minimum for all samples through 9,216 routes: (pop, chunk, seq) for L1, L2, L3.
BASE = (16, 4, 15, 32, 8, 9, 2, 1, 1)


def main() -> None:
    """Try each placement and print the best allocation."""
    tables: list[tuple[int, ...]] = []
    with (CACHE_STATS_DIR / "build" / ALIGNED_CSV).open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            if row["table_size"] == "9728":
                tables.append(tuple(int(row[bank]) // RAMB18_BYTES for bank in ALIGNED_COLUMNS))
    if not tables:
        raise SystemExit("No 9,728-route tables in the aligned CSV")

    best_count = -1
    best: tuple[int, ...] = BASE
    # Banks 0 and 1 are fixed. Swapping the two additions gives the same allocation.
    for first in range(2, 9):
        for second in range(first, 9):
            candidate = list(BASE)
            candidate[first] += 1
            candidate[second] += 1

            # L2 and L3 pop banks grow by one RAMB36 (two RAMB18 equivalents).
            if candidate[3] % 2 or candidate[6] % 2:
                continue

            count = 0
            for table in tables:
                if all(need <= capacity for need, capacity in zip(table, candidate, strict=True)):
                    count += 1
            if count > best_count:
                best_count = count
                best = tuple(candidate)

    total = len(tables)
    print(f"Best allocation: {best}")
    print(f"9,728-route tables covered: {best_count:,}/{total:,} ({best_count / total:.2%})")


if __name__ == "__main__":
    main()
