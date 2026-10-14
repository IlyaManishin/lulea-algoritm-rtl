# Table Generation & Batch Sampling Pipeline

Utilities for generating and batch-processing routing tables used in Luleå trie hardware verification and memory footprint analysis.

---

## Architecture Overview

The toolchain employs a two-stage pipeline to convert large binary BGP routing tables (~1M entries each) into sized, stratified topology samples:

```
[ Binary MRT Archives ] (.bz2 / .gz / uncompressed)
        │
        ▼  Stage 1: batch_convert.py (pipeline/convert.py)
[ build/converted/<source>/<stem>/ ]
  ├── routes.txt     (Full table: prefix + next_hop)
  └── routes.ports    (Full port mapping)
        │
        ▼  Stage 2: batch_sample.py (pipeline/sample.py)
[ build/sampled/<size>/<source>/<stem>/seed_<N>/ ]
  ├── routes.txt     (Sampled table: prefix + next_hop)
  └── routes.ports    (Sampled port mapping)
```

---

## Two-Stage Batch Pipeline

### Stage 1: Batch Convert (`batch_convert.py`)

Parses raw binary MRT dumps (RouteViews `.bz2`, RIPE RIS `.gz`, or plain MRT) into an intermediate plain-text format.

- **Idempotent**: Skips archives whose output directory already contains non-empty results unless `--force` is specified.
- **Decompression**: Automatically decompresses archives to a temporary file, parses records using `bgpkit-parser`, and cleans up.
- **Output Hierarchy**:
  ```
  build/converted/
  └── <source>/
      └── <mrt_stem>/
          ├── routes.txt
          └── routes.ports
  ```

### Stage 2: Batch Sample (`batch_sample.py`)

Samples converted full-tables into multiple fixed-size sub-tables (`1024`, `2048`, `4096`, etc.) with varying random seeds.

- **Stratified Sampling**: Preserves the original IPv4 prefix length distribution using `filter_mrt_routes()`.
- **Sample Count Formula**:
  $$\text{samples} = \left\lfloor \frac{\text{total\_routes} \times \text{coeff}}{\text{target\_size}} \right\rfloor$$
  The parameter `coeff` (`SAMPLE_FILL_COEFF`) controls table reuse (e.g. `1.0` extracts non-overlapping volume equivalent to the full table, while `2.0` reuses routes ~2x across samples).
- **Memory Optimization**: `SampleExecutor` maintains a single-entry lazy cache of the active table, avoiding redundant disk I/O while iterating across sizes and seeds.
- **Progress Monitoring**: Powered by `rich` with live ETA, elapsed time, and task counters.
- **Output Hierarchy**:
  ```
  build/sampled/
  └── <size>/
      └── <source>/
          └── <mrt_stem>/
              └── seed_<N>/
                  ├── routes.txt
                  └── routes.ports
  ```

---

## File Formats

All text files include a count header on the first line for $O(1)$ size inspection and file integrity validation.

### Intermediate Routes (`routes.txt`)
```text
1053412
1.0.0.0/24 203.0.113.1
1.0.4.0/22 198.51.100.2
...
```

### Port Map (`routes.ports`)
```text
247
203.0.113.1 1
198.51.100.2 2
...
```

---

## Module Layout

- **`config.py`**: Pipeline directories, target sizes, default fill coefficients, and environment paths.
- **`pipeline/`**:
  - `convert.py`: Stage 1 logic (discovery, decompression, `ConvertResult` tracking).
  - `sample.py`: Stage 2 logic (`SampleTask`, `SamplePlan`, `SampleExecutor` with lazy route caching).
  - `route_table_gen.py`: Single-table generator workflow (random or single MRT).
- **`core/`**:
  - `gen_types.py`: Core dataclasses (`RouteRecord`).
  - `mrt_reader.py`: Binary and text MRT parsing via `bgpkit`.
  - `mrt_gen.py`: Stratified filtering logic.
  - `random_gen.py`: Synthetic route generation based on empirical IPv4 prefix distributions.
- **`utils/`**:
  - `port_mapping.py`: Next-hop IP to numeric port ID mapping.
  - `route_saver.py`: Serialisation for raw routes, compiled route tables, and port maps.
  - `route_reader.py`: Parsing and validation for files containing count headers.
- **CLI Entrypoints**:
  - `batch_convert.py`: CLI driver for Stage 1.
  - `batch_sample.py`: CLI driver for Stage 2.
  - `main.py`: Standalone CLI for generating single custom or random tables.

---

## Usage Examples

### 1. Batch Convert (Stage 1)
```bash
# Convert all MRT files located in MRT_SOURCE_DIR (configured via .env)
python batch_convert.py

# Specify explicit source and output directories
python batch_convert.py --source /path/to/mrt_dumps --output build/converted

# Force re-conversion of existing outputs
python batch_convert.py --force
```

### 2. Batch Sample (Stage 2)
```bash
# Sample using default sizes [1024, 2048, 4096, 8192, 16384] and coeff=1.0
python batch_sample.py

# Custom target sizes and fill coefficient
python batch_sample.py --sizes 1024 2048 --coeff 2.0

# Force re-sampling
python batch_sample.py --force
```

### 3. Standalone Table Generation
```bash
# Generate 4096 random routes
python main.py -t random -c 4096 -o build/random_routes.txt

# Extract 1024 routes from a single binary MRT dump
python main.py -t mrt -i /path/to/dump.mrt -f binary -c 1024 -o build/routes.txt
```
