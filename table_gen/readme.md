# Table Generation & Batch Sampling Pipeline

Utilities for generating and batch-processing routing tables used in Luleå trie hardware verification and memory footprint analysis.

---

## Architecture Overview

The toolchain employs a two-stage pipeline to convert large binary BGP routing tables (~1M entries each) into sized, stratified topology samples:

```
[ Binary MRT Archives ] (.bz2 / .gz / uncompressed)
        │
        ▼  Stage 1: batch_convert.py (engine/pipeline/convert.py)
[ build/converted/<source>/<stem>/ ]
  ├── routes.txt      (Full table: count header + "prefix port_id")
  ├── routes.ports    (Full port mapping: count header + "port_id next_hop")
  └── info.txt        (Conversion statistics and IPv6 filter counts)
        │
        ▼  Stage 2: batch_sample.py (engine/pipeline/sample.py)
[ build/sampled/<size>/<source>/<stem>/seed_<N>/ ]
  ├── routes.txt      (Sampled table: compact binary LRB1)
  └── routes.ports    (Sampled port mapping: compact binary LPB1)
```

---

## Two-Stage Batch Pipeline

### Stage 1: Batch Convert (`batch_convert.py`)

Parses raw binary MRT dumps (RouteViews `.bz2`, RIPE RIS `.gz`, or plain MRT) into an intermediate plain-text format.

- **Multithreading**: Files are processed concurrently using a thread pool configured via `CONVERT_WORKERS` in `engine/config.py`.
- **Instant Shutdown on Ctrl+C**: Aborts workers immediately on `KeyboardInterrupt` with clean error code exit (`130`).
- **Streaming Decompression**: Bypasses temporary disk files completely. Archives are decompressed in-memory on the fly by `bgpkit-parser` in Rust with micro-buffers, preventing RAM exhaustion.
- **Strict IPv4 & RFC 5549 Filtering**: Automatically drops IPv6 prefixes and IPv4 prefixes carrying RFC 5549 IPv6 next hops (`_is_ipv6_route`), ensuring only valid IPv4 unicast routes enter the pipeline.
- **Conversion Report (`info.txt`)**: Logs total records scanned, unique IPv4 routes, unique ports, and count of discarded IPv6/duplicate entries.
- **Idempotent**: Skips archives whose output directory already contains non-empty results unless `--force` is specified.
- **Output Hierarchy**:
  ```
  build/converted/
  └── <source>/
      └── <mrt_stem>/
          ├── routes.txt
          ├── routes.ports
          └── info.txt
  ```

### Stage 2: Batch Sample (`batch_sample.py`)

Samples converted full-tables into multiple fixed-size sub-tables (`1024`, `2048`, `4096`, `8192`, `16384`) across random seeds.

- **Multiprocessing per Source**: Tasks are grouped by source file (`FileSampleGroup`). The source table and port mapping are loaded into memory once per group, and sample generation (`seed` + `target_size`) is executed concurrently across processes using `SAMPLE_WORKERS` workers. Memory is shared via OS fork Copy-on-Write (CoW) without IPC/pickle overhead, fully bypassing Python's GIL.
- **Stratified Hamilton Sampling**: Uses the mathematically exact **Hamilton (Largest Remainder)** method in `engine/core/mrt_filter.py` to preserve the source prefix length distribution without rare-stratum inflation or truncation distortion:
  1. Partitions routes into mask strata ($m \in [1..32]$).
  2. Allocates integer base quotas: $\lfloor K \times \frac{N_m}{N} \rfloor$.
  3. Distributes the remaining quota deficit to strata with the largest fractional remainders.
  4. Uniformly samples routes within each stratum.
- **Sample Count Formula**:
  $$\text{samples} = \left\lfloor \frac{\text{total\_routes} \times \text{coeff}}{\text{target\_size}} \right\rfloor$$
  The parameter `coeff` (`SAMPLE_FILL_COEFF`, default `1.0`) controls table reuse. Across independent seeds, $c=1.0$ statistically samples ~63.2% unique routes from the full table.
- **Atomic File Writes**: Serializers in `engine/utils/route_saver.py` write to `.tmp.<pid>` files first before calling `os.replace`, guaranteeing that files are never left half-written or corrupted if interrupted.
- **Progress Monitoring**: Powered by `rich` with live ETA, elapsed time, and task counters. Exits with status code `1` if any errors occur.
- **Output Hierarchy**:
  ```
  build/sampled/
  └── <size>/
      └── <source>/
          └── <mrt_stem>/
              └── seed_<N>/
                  ├── routes.txt      (Binary LRB1)
                  └── routes.ports    (Binary LPB1)
  ```

---

## File Formats

### Text Formats (Stage 1 converted tables)

All text files include a count header on the first line for $O(1)$ size inspection.

#### Routes Table (`routes.txt`)
Stores `<prefix> <port_id>`:
```text
1053412
1.0.0.0/24 1
1.0.4.0/22 2
...
```

#### Port Map (`routes.ports`)
Stores `<port_id> <next_hop>`:
```text
247
1 203.0.113.1
2 198.51.100.2
...
```

### Binary Formats (Stage 2 sampled tables)

Sampled tables use compact binary formats with magic headers. `read_routes()`, `read_port_map()`, and `read_route_count()` automatically detect the format by inspecting the first 4 bytes.

#### Binary Routes (`LRB1`)
- **Header** (8 bytes): `b"LRB1"` + `count: uint32` (Big-Endian)
- **Records** (8 bytes per entry, struct `>IBHx`):
  - `ip_int: uint32` (BE) — IPv4 prefix base as integer
  - `mask: uint8` (1..32) — Prefix mask length
  - `port_id: uint16` (BE) — Hardware egress port identifier
  - `pad: uint8` (0x00) — Alignment padding

#### Binary Port Map (`LPB1`)
- **Header** (8 bytes): `b"LPB1"` + `count: uint32` (BE)
- **Records** (6 bytes per entry, struct `>HI`):
  - `port_id: uint16` (BE)
  - `ip_int: uint32` (BE) — IPv4 next-hop address as integer

### Conversion Report (`info.txt`)
```text
source_file: rib.20241001.0000.bz2
total_elements: 18942689
unique_routes: 1053412
unique_ports: 247
filtered_ipv6: 312450
skipped_elements: 17576827
```

---

## Package Architecture & Module Layout

The codebase cleanly separates **CLI entry points** from the **reusable domain engine**:

```text
table_gen/
├── batch_convert.py            # CLI: Stage 1 (MRT archives -> intermediate tables)
├── batch_sample.py             # CLI: Stage 2 (intermediate tables -> sampled binary tables)
├── main.py                     # CLI: Standalone single-table generator (random or MRT)
├── readme.md                   # Documentation
└── engine/                     # Reusable library package (PEP 420 namespace, 100% relative imports)
    ├── config.py               # Hardware parameters, binary struct constants, and paths
    ├── core/
    │   ├── gen_types.py        # Immutable dataclasses (RouteRecord, MrtRecord, PortMapping)
    │   ├── mrt_filter.py       # Hamilton stratified sampling algorithm
    │   ├── mrt_gen.py          # MRT route generation workflows
    │   ├── mrt_reader.py       # Streaming MRT reader & RFC 5549 IPv6 next-hop filtering
    │   └── random_gen.py       # Synthetic IPv4 prefix generator with bounded port pools
    ├── pipeline/
    │   ├── convert.py          # Stage 1 conversion orchestration & thread pool
    │   ├── route_table_gen.py  # Standalone single-table pipeline
    │   └── sample.py           # Stage 2 sampling orchestration & process pool
    └── utils/
        ├── concurrency.py      # ThreadPool & fork ProcessPool runners with Ctrl+C abort
        ├── port_mapping.py     # Next-hop to port mapping (hardware limit: 255 ports)
        ├── route_reader.py     # Binary & text parser with auto-format detection
        └── route_saver.py      # Binary & text atomic serializers
```

### Pure Relative Imports Inside `engine/`
Modules within `engine/` use standard relative imports (`from ..config import ...`, `from .gen_types import ...`), ensuring the engine is completely independent of `sys.path`.

### Hardware Constraints
- `PORT_SIZE_BITS = 8`: Fixed by RTL hardware lookup specifications.
- `PORTS_COUNT_MAX = 255`: Synthetic generator (`random_gen.py`) samples a bounded pool of ports upfront ($16 \le P \le 255$) to avoid port exhaustion during random generation.

---

## External Usage (Python API)

Other tools in the repository (e.g. golden models, hardware testbenches, stats scripts) can import library modules directly:

```python
from table_gen.engine.utils.route_reader import read_routes, read_ports
from table_gen.engine.utils.route_saver import save_routes, save_port_map, FileFormat
from table_gen.engine.core.gen_types import RouteRecord, PortMapping
from table_gen.engine.config import PORTS_COUNT_MAX

# Read existing binary or text table:
routes: list[RouteRecord] = read_routes("build/sampled/1024/.../seed_42/routes.txt")
print(f"Loaded {len(routes)} routes; first route: {routes[0].prefix} -> port {routes[0].port_id}")

# Save in binary format:
save_routes(routes, "/tmp/test_routes.bin", file_format=FileFormat.BINARY)
```

---

## CLI Usage Examples

### 1. Batch Convert (Stage 1)
```bash
# From inside table_gen/:
cd table_gen
python batch_convert.py

# Or from repository root:
python table_gen/batch_convert.py

# Specify explicit source and output directories
python table_gen/batch_convert.py --source /path/to/mrt_dumps --output build/converted

# Force re-conversion of existing outputs
python table_gen/batch_convert.py --force
```

### 2. Batch Sample (Stage 2)
```bash
# From inside table_gen/:
cd table_gen
python batch_sample.py

# Or from repository root:
python table_gen/batch_sample.py

# Custom target sizes and fill coefficient
python table_gen/batch_sample.py --sizes 1024 2048 --coeff 2.0

# Force re-sampling
python table_gen/batch_sample.py --force
```

### 3. Standalone Table Generation
```bash
# Generate 4096 random routes with port mapping:
python table_gen/main.py -t random -c 4096 -o build/routes.txt

# Extract 1024 routes from a single binary MRT dump:
python table_gen/main.py -t mrt -i /path/to/dump.mrt -c 1024 -o build/routes.txt
```
