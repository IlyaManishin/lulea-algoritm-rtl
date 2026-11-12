# cache_gen

C++20 project (library + CLI tool `lulea_builder`) for building a 3-level Lulea routing tree (default `18-6-8` split: L1 = 18 bits, L2 = 6 bits, L3 = 8 bits) and generating hardware caches (`pop`, `chunk`, `seq` for L1/L2/L3) for RTL.

---

## Project Structure

```text
cache_gen/
├── meson.build                 # Build and test config
├── meson.options               # Option strict_flags (GCC -fanalyzer, -Werror, etc.)
├── .clang-tidy                 # clang-tidy rules (WarningsAsErrors: '*')
├── scripts/run_linters.sh      # Helper script for cppcheck and clang-tidy
├── src/
│   ├── main.cpp                # CLI entry point (lulea_builder)
│   ├── io/                     # CLI args, TSV config parser, stats output
│   └── gen/
│       ├── gen.{hpp,cpp}       # Top-level build_lulea_cache pipeline (stub)
│       ├── config.hpp          # Core types (ipv4_t, mask_t, port_t, NULL_PORT)
│       ├── lulea_config.hpp    # 18-6-8 scheme constants and cell bit widths
│       ├── utils/rtl.hpp       # RTL helper math (clog2, BRAM alignment, etc.)
│       ├── core/
│       │   ├── cache.*         # BaseCell, LuleaCell, LuleaRoute, LuleaOverflow
│       │   ├── lulea_tree.*    # 3-level LuleaTree builder and lookup (LPM)
│       │   └── lulea_algo.*    # LuleaAlgo skeleton (tree init and cache generation)
│       └── io/
│           ├── reader.*        # Binary route table reader
│           └── bit_file_writer.* # Bit-level writer for cache dump files
└── tests/                      # GoogleTest unit tests (target: test_gen)
```

---

## Quick Cheat Sheet

Run these commands from `cache_gen/` (if you run from the repo root, use `-C cache_gen/build` and `./cache_gen/...`).

### 1. Setup and Build (Meson + Ninja)

```bash
cd cache_gen

# Default setup (Release: optimization=3, C++20)
meson setup build

# Debug build
meson setup build-debug -Dbuildtype=debug

# Strict GCC checks + -Werror + -fanalyzer (optional)
meson setup build-strict -Dstrict_flags=true
```

> **RAM tip:** `-Dstrict_flags=true` turns on `-fanalyzer`. Compiling `gtest` tests in parallel uses a lot of RAM, so limit jobs with `-j 2`.

```bash
# Build everything (lulea_builder + test_gen)
meson compile -C build -j 2

# Build only the binary (skip tests)
meson compile -C build -j 2 lulea_builder
```

### 2. Run Tests

```bash
# Run all tests via Meson
meson test -C build -v

# Run test_gen directly (good for GTest filters)
./build/test_gen
./build/test_gen --gtest_filter="LuleaTreeTest.*"
./build/test_gen --gtest_filter="*Overflow*"
./build/test_gen --gtest_list_tests
```

### 3. Run the Binary (`lulea_builder`)

```bash
# Basic run (prints stats to stdout)
./build/lulea_builder path/to/config.tsv

# Save stats to a CSV file (--batch flag is optional)
./build/lulea_builder --batch path/to/config.tsv --out-csv results.csv
```

- **`config.tsv` format**: tab-separated lines (`#` comments and blank lines are skipped):
  ```tsv
  # mode	input_path	[dest_dir]
  stats	routes_1.bin	-
  dump	routes_2.bin	out/caches
  ```
  - `mode`: `stats` or `dump`
  - `dest_dir`: optional (`-` means no output folder)
- **CSV columns** (`--out-csv`): `file,l1_pop,l1_chunk,l1_seq,l2_pop,l2_chunk,l2_seq,l3_pop,l3_chunk,l3_seq`.

### 4. Run Linters (`cppcheck` and `clang-tidy`)

Linters need `compile_commands.json` inside the build folder (`meson setup` creates it).

```bash
# Run both linters (uses ./build by default)
./scripts/run_linters.sh

# Only cppcheck (checks all files in compile_commands.json, including tests/)
./scripts/run_linters.sh --cppcheck-only

# Only clang-tidy (checks src/**/*.cpp, warnings are errors)
./scripts/run_linters.sh --clang-tidy-only

# Use another build directory
./scripts/run_linters.sh --build-dir lint
```

Or run them via Meson targets:

```bash
meson compile -C build lint
meson compile -C build lint-cppcheck
meson compile -C build lint-clang-tidy
```
