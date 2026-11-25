# Statistics

Run these tools from the repository root.

## Memory estimation

`mem_estimation.py` models how BRAM usage grows with the route limit for the
Luleå address splits `16-8-8`, `18-6-8`, and `20-4-8`. It evaluates powers
of two from 1,024 to 2,097,152 routes using the prefix proportions and chunk
sizes defined for each split. [Layout definitions](lulea_configurations.py)

For each limit, the model estimates L2 and L3 node counts by assuming prefixes
are distributed uniformly among the available parent slots. It uses these
counts to size pointers and estimate the lengths of the reference sequences.
It then adds up the bitmap, chunk sums, and reference sequence at each level.
By default, node counts round up to powers of two and each array rounds up
to an 18-Kibit BRAM boundary.

```sh
venv/bin/python stats/mem_estimation.py
```

The script writes these reports to `stats/build/mem_estimation/`.

| Report | Use |
|---|---|
| `configs_totals.csv` | Compare total estimated memory across all splits and route limits. |
| `<layout>.csv` | Inspect one split across all limits, including its per-level memory breakdown. |
| `limits_out/limit_<N>.csv` | Compare all splits at one route limit. |

The `_short.csv` variants keep the main columns. Columns ending in `_kb` are
in KiB.

## Route and port data

`mrt_analyzer.py` summarizes prefix lengths and ports or next hops. It detects
MRT dumps, text and binary route tables, and port maps by their contents.

```sh
venv/bin/python stats/mrt_analyzer.py PATH
```

## Cache measurements and allocation

Build `lulea_builder`, then measure generated caches for the sampled routing
tables and search for an allocation within the BRAM budget.
[Build instructions](../cache_gen/README.md#quick-start)

```sh
python3 stats/cache_stats/collect_stats.py
python3 stats/cache_stats/search_allocation.py
```

The collector reads `tables_build/sampled/` and writes its reports to
`stats/cache_stats/build/`. The search uses the full aligned CSV.
[CSV formats and allocation results](cache_stats/README.md)
