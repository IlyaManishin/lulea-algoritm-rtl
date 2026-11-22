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

The cache budget is 90 RAMB18 equivalents (202.5 KiB).
Each RAMB36 counts as two RAMB18s.
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

Sizing the banks to these requirements uses **81 of 90 RAMB18 equivalents**
(182.25 KiB), leaving **9 RAMB18 equivalents (20.25 KiB)** free.

The allocation also leaves unused payload bits inside the banks. The ranges
below are the minimum and maximum across the 4,262 samples.

| Level | Unused `pop` bits | Unused `chunk` bits | Unused `seq` bits |
| --- | ---: | ---: | ---: |
| L1 | 0 | 0 | 9,396–17,784 |
| L2 | 1,728–16,192 | 486–4,554 | 5,958–8,181 |
| L3 | 9,216–28,928 | 11,808–17,352 | 16,695–18,216 |

Each sample leaves **65,960–105,882 payload bits** unused across its nine
banks. The 64-bit bitmap layout also leaves 94,208 physical bits outside the
configured data width. When allocating the nine free blocks, use each bank's
minimum spare capacity to see how much that bank can grow before it needs
another block. Spare capacity belongs to the bank that holds it.

### 90-block configuration

The trial allocation is `(16, 4, 16, 32, 8, 10, 2, 1, 1)` in RAMB18
equivalents, ordered by level and then by `pop`, `chunk`, and `seq`. The
extra block goes to `l2_seq` and adds 2,048 entries. On the intermediate-size
samples from 33 MRT snapshots, this allocation fits 165 of 165 tables at
9,216 routes, 105 of 165 at 9,728, and 0 of 165 at 10,240.
