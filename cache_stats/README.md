# Cache statistics

From the repository root, run `python3 cache_stats/collect_stats.py` with
[`lulea_builder` built](../cache_gen/README.md#quick-start).
The collector lists the tables in `build/sampled` and writes four files under
`cache_stats/build/`.

- `jobs.tsv` contains one `stats` job per sampled routing table.
- `cache_sizes_packed.csv` records the nine packed bank sizes in bytes for every table.
- `cache_sizes.csv` records the nine BRAM-aligned bank sizes in bytes for every table.
- `cache_sizes_in_budget.csv` keeps the aligned rows whose nine sizes sum to
  at most the BRAM budget.

Each CSV includes the path relative to `build/sampled` and the route count from
the size directory.

## Allocation experiment

The cache budget is 89 RAMB18 equivalents (200.25 KiB), rounding 200 KiB up
to the next 2.25 KiB block. Each RAMB36 counts as two RAMB18s.
For each table size, divide the number of tables that fit by the total number
sampled at that size. The configuration score is the sum of these fractions.
Use the full aligned CSV for group totals so that tables over budget still count.

### 8,192-route tables

All 4,262 sampled tables at this size have the same BRAM requirements. Their
packed sizes vary, but rounding each bank up to a whole BRAM block yields the
same allocation for every sample. The table lists RAMB18 equivalents.

| Level | `pop` | `chunk` | `seq` | Total |
| --- | ---: | ---: | ---: | ---: |
| L1 | 16 | 4 | 14 | 34 |
| L2 | 28 | 7 | 8 | 43 |
| L3 | 2 | 1 | 1 | 4 |
| **Total** | **46** | **12** | **23** | **81** |

Sizing the banks to these requirements uses **81 of 89 RAMB18 equivalents**
(182.25 KiB), leaving **8 RAMB18 equivalents (18 KiB)** free.
