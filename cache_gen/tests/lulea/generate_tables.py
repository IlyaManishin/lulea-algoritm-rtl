#!/usr/bin/env python3
"""Generate sampled routing tables from clean converted MRT tables for C++ tests.

Reads a converted routing table (from build/converted/...), performs Hamilton
stratified sampling to size 2k (2048 routes), and saves both human-readable
text and compact binary LRB1 representations.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import sys
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[2]
TABLE_GEN_DIR = REPO_ROOT / "table_gen"

for p in (REPO_ROOT, TABLE_GEN_DIR):
    if str(p) not in sys.path:
        sys.path.insert(0, str(p))

from engine.core.gen_types import PortMapping, RouteRecord  # noqa: E402
from engine.core.mrt_filter import filter_mrt_routes  # noqa: E402
from engine.utils.route_reader import read_port_map, read_routes  # noqa: E402
from engine.utils.route_writer import FileFormat, save_port_map, save_routes  # noqa: E402

DEFAULT_CONVERTED_SRC = (
    REPO_ROOT
    / "build"
    / "converted"
    / "ris_ripe_net"
    / "rrc00_bview_20250601_0000"
    / "routes.txt"
)
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "tables"


def filter_port_map_for_routes(
    routes: list[RouteRecord], port_map: PortMapping | None
) -> PortMapping | None:
    if not port_map:
        return None
    used_ports = {r.port_id for r in routes}
    return {p_id: nh for p_id, nh in port_map.items() if p_id in used_ports}


def write_table_dir(
    target_dir: Path,
    routes: list[RouteRecord],
    port_map: PortMapping | None,
    meta: dict[str, Any],
) -> None:
    target_dir.mkdir(parents=True, exist_ok=True)

    txt_path = target_dir / "routes.txt"
    bin_path = target_dir / "routes.bin"
    save_routes(routes, txt_path, file_format=FileFormat.TEXT)
    save_routes(routes, bin_path, file_format=FileFormat.BINARY)

    # Verification read-back
    back_txt = read_routes(txt_path)
    back_bin = read_routes(bin_path)
    if len(back_txt) != len(routes) or len(back_bin) != len(routes):
        raise RuntimeError(f"Length mismatch in {target_dir}")
    for idx, (exp, act) in enumerate(zip(routes, back_bin)):
        if exp.prefix != act.prefix or exp.port_id != act.port_id:
            raise RuntimeError(f"Mismatch at index {idx} in {target_dir}")

    # Port mapping if available
    filtered_ports = filter_port_map_for_routes(routes, port_map)
    if filtered_ports:
        save_port_map(filtered_ports, target_dir /
                      "routes.ports", file_format=FileFormat.TEXT)
        save_port_map(filtered_ports, target_dir /
                      "routes.ports.bin", file_format=FileFormat.BINARY)

    # Info summary
    mask_dist = Counter(r.mask for r in routes)
    top_masks = ", ".join(f"/{m}:{cnt}" for m, cnt in mask_dist.most_common(8))
    info_content = (
        f"table_name: {target_dir.name}\n"
        f"routes_count: {len(routes)}\n"
        f"unique_prefixes: {len({r.prefix for r in routes})}\n"
        f"unique_ports: {len({r.port_id for r in routes})}\n"
        f"source_file: {meta.get('source_file', '')}\n"
        f"seed: {meta.get('seed', '')}\n"
        f"mask_distribution: {top_masks}\n"
    )
    (target_dir / "info.txt").write_text(info_content, encoding="utf-8")


def generate(
    source_file: Path,
    output_dir: Path,
    count: int = 2048,
    num_tables: int = 10,
    base_seed: int = 42,
) -> None:
    print(f"Reading clean converted routes from: {source_file}")
    routes = read_routes(source_file)
    print(f"Total routes loaded: {len(routes)}")

    port_map: PortMapping | None = None
    ports_candidate = source_file.with_name("routes.ports")
    if ports_candidate.is_file():
        port_map = read_port_map(ports_candidate)
        print(f"Loaded port map with {len(port_map)} ports")

    output_dir.mkdir(parents=True, exist_ok=True)
    for i in range(num_tables):
        idx = i + 1
        seed = base_seed + i * 137
        sampled = filter_mrt_routes(routes, target_count=count, seed=seed)

        target_dir = output_dir / f"table_{idx}"
        meta = {
            "source_file": str(source_file),
            "seed": seed,
        }
        write_table_dir(target_dir, sampled, port_map, meta)
        unique_ports = len({r.port_id for r in sampled})
        print(f"  [{idx}/{num_tables}] -> {target_dir.name} ({len(sampled)} routes, {unique_ports} ports, seed={seed})")

    print(f"\nAll {num_tables} tables generated in: {output_dir}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate 2k test tables from converted MRT tables.")
    parser.add_argument("-i", "--input", type=Path, default=DEFAULT_CONVERTED_SRC,
                        help="Source routes.txt in build/converted/")
    parser.add_argument("-o", "--output-dir", type=Path,
                        default=DEFAULT_OUTPUT_DIR, help="Destination directory for tables")
    parser.add_argument("-c", "--count", type=int,
                        default=2048, help="Number of routes per table")
    parser.add_argument("-n", "--num-tables", type=int,
                        default=10, help="Number of tables")
    parser.add_argument("-s", "--seed", type=int,
                        default=42, help="Base random seed")
    args = parser.parse_args()

    if not args.input.is_file():
        print(f"Error: input file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    generate(
        source_file=args.input,
        output_dir=args.output_dir,
        count=args.count,
        num_tables=args.num_tables,
        base_seed=args.seed,
    )


if __name__ == "__main__":
    main()
