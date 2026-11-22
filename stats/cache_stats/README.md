# Cache statistics

The experiment chooses one fixed allocation for the nine Luleå cache banks
on the FPGA. It uses routing tables sampled across 33 MRT snapshots to measure
how many tables fit within a budget of 90 RAMB18 equivalents (202.5 KiB).
A table fits when all nine banks have enough space for their own data.

## Input and reports

`collect_stats.py` scans the sampled routing tables in `build/sampled/` and
passes them to `lulea_builder` in stats mode. Each CSV row records a table's
path, route count, and nine BRAM-aligned bank sizes in bytes. The reports are in
`stats/cache_stats/build/`.

| Report | Use |
|---|---|
| `cache_sizes.csv` | Measure the block capacity required by each bank. The allocation search uses this full report to count coverage across all samples. |
| `cache_sizes_in_budget.csv` | Find tables whose nine aligned sizes sum to at most 90 RAMB18 equivalents. This gives an upper bound on how many tables any fixed allocation can fit. |

The collector also saves its input job list as `jobs.tsv`.
[Run the collector and search](../README.md#cache-measurements-and-allocation)

## Allocation

The 88-block allocation fits every sampled table through 9,216 routes, leaving
two blocks. The search script tries where to place them and scores each
allocation on the 9,728-route samples. [Search script](search_allocation.py)

| Allocation | L1 | L2 | L3 | RAMB18 equivalents |
|---|---|---|---|---:|
| Through 9,216 routes | (16, 4, 15) | (32, 8, 9) | (2, 1, 1) | 88 |
| Best at 9,728 routes | (16, 4, 16) | (32, 8, 10) | (2, 1, 1) | 90 |

Each triple lists the `(pop, chunk, seq)` banks. The best allocation adds one
block to `l1_seq` and one to `l2_seq`, covering **2,206 of 3,589 tables (61.47%)**
at 9,728 routes.

## Coverage by table size

BRAM requirements are in RAMB18 equivalents. A RAMB36 counts as two.

| Routes | Tables | BRAM required per table | Tables that fit |
|---:|---:|---:|---:|
| 1,024 | 34,234 | 32 | 34,234 |
| 2,048 | 17,107 | 40 | 17,107 |
| 4,096 | 8,544 | 56 | 8,544 |
| 8,192 | 4,262 | 81 | 4,262 |
| 8,704 | 4,013 | 85–86 | 4,013 |
| 9,216 | 3,787 | 86–88 | 3,787 |
| 9,728 | 3,589 | 89–93 | 2,206 |
| 10,240 | 3,408 | 93–94 | 0 |
| 16,384 (earlier run) | 2,122 | 125–129 | 0 |

The sampling step is 512 routes between 8,192 and 10,240. The full report includes the existing 16,384-route samples.
