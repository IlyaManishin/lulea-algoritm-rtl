# Luleå verification

`liblulea_verify.a` exposes `verify_lookup_engine` through
`<lulea_verify/lookup_verification.hpp>` and `lulea_verify_dep` in Meson.
The lookup engine and factory interfaces are in `<lulea_verify/lookup_engine.hpp>`.
The LRB1 reader and route types belong to `liblulea_common.a` under `<lulea_common/...>`.

## Build and test

Run from the repository root with C11 and C++20 compilers, Meson 1.1 or
newer, Ninja, GoogleTest, and Python 3.12 or newer with `python-dotenv`.
Meson uses `venv/bin/python` when available and `python3` from PATH otherwise.

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

Building the static library generates two shared tables of 9,216 routes
under `build/libs_cpp/lulea_verify/tables/`. Meson runs the C++ verification tests
from `build/libs_cpp/lulea_verify/`. Each table directory contains `routes.txt` and
`routes.bin`.

The build samples the default converted MRT snapshot under `tables_build/converted/`
when available and uses deterministic synthetic fixtures otherwise. Use
`-Dverify_routes=/path/to/routes.txt` to select a converted table,
`-Dverify_route_count` to set the size, and `-Dverify_table_count` to set
the number of fixtures. The golden model requires every fixture to fit its
[fixed BRAM banks](../../stats/cache_stats/TOTALS.md).
