# Cache Generator

`lulea_builder` turns IPv4 routing tables into packed memory images for a three-level **Luleå longest-prefix-match lookup**. It targets the `18–6–8` layout and reports both file sizes and estimated block RAM usage for Artix-7.

| | |
| --- | --- |
| **Input** | IPv4 prefixes and 8-bit output ports in [LRB1 format](../table_gen/readme.md#binary-routes-lrb1) |
| **Address split** | L1 `ip[31:14]` · L2 `ip[13:8]` · L3 `ip[7:0]` |
| **Output** | Nine binary files, three per level |
| **Verification** | Tree and cache lookups checked against a linear longest-prefix-match reference |

## Quick start

Build from the repository root with the [shared Meson project](../libs_cpp/lulea_verify/README.md).

```bash
meson setup build
meson compile -C build cache_gen
```

With an LRB1 file named `routes.bin`, write a tab-separated job file and run the builder.

```bash
mkdir -p out
printf 'dump\t%s\t%s\n' routes.bin out/cache > jobs.tsv
./build/cache_gen/lulea_builder jobs.tsv --out-csv out/sizes.csv
```

Each row contains a mode, an input path, and an output directory for `dump`. Use `stats` with just the input path to measure a table without writing cache files. Paths are relative to the working directory. Omit `--out-csv` to print the sizes to stdout.

Add `--progress` to show the number of processed jobs on stderr.

## Cache layout

Each level has three files, with `lN` standing for `l1`, `l2`, or `l3`.

| File | Entry width | Contents |
| --- | ---: | --- |
| `lN_pop.bin` | 64 bits | Bitmap chunks marking where the port or pointer changes |
| `lN_chunk.bin` | 18 bits | Count of set bits before each bitmap chunk |
| `lN_seq.bin` | 18 bits at L1, 9 bits at L2 and L3 | Port IDs and next-level block indices |

Entries are packed least significant bit first, with padding only at the end of each file. The first `seq` entry holds the initial value for the level. Lookup adds the chunk sum to the popcount through the selected bit, inclusive, to index `seq`. At L1 and L2, the high bit is zero for a port and one for a pointer. L3 stores ports only, and the L2 pointer can address up to 256 L3 blocks. [Cache layout and lookup datapath](../docs/implementation_18_6_8.drawio)

## BRAM usage

The CSV lists packed sizes in bytes, followed by the corresponding `_aligned` columns. The estimate allocates each array separately using RAMB36E1 for 64-bit bitmap chunks and RAMB18E1 for the 18- and 9-bit arrays.

| RIPE sample | Packed files | Estimated BRAM |
| --- | ---: | ---: |
| 4,096 routes | 103.5 KiB | 126 KiB |
| 16,384 routes | 253.7–254.3 KiB | 283.5 KiB |

The 4,096-route sample uses 56% of the Basys 3's 225 KiB BRAM budget. The two 16,384-route samples exercise the software lookup and exceed that budget. Usage depends on the prefixes and ports in the table.

## Verification

The two lookup tests compare against a linear scan using prefix boundaries,
random addresses, and a sweep across IPv4 space. The shared verification suite
also compares every BIN record with its TXT counterpart.
[Fixture configuration](../libs_cpp/lulea_verify/README.md)

```bash
meson test -C build --suite cache_gen --print-errorlogs
meson compile -C build lint-cache-gen
```

The `lint-cache-gen` target runs `cppcheck` and `clang-tidy` for this component.

## Project layout

```text
cache_gen/
├── src/main.cpp       CLI entry point
├── src/io/            Job parsing and statistics output
├── src/gen/
│   ├── lulea_algo/    Tree construction and cache generation
│   ├── lulea_data/    Route cells, caches, and BRAM accounting
│   └── io/            Packed binary writer
├── tests/gen/         Unit tests and synthetic lookups
└── tests/lulea/       Verification and software lookup engines
```
