#!/usr/bin/env python3
"""Generate shared route tables for the C++ verification tests.

Sample a converted MRT table or generate deterministic synthetic routes.
Write each fixture as text and as packed LRB1.
"""

from __future__ import annotations

import argparse
from collections import Counter
from ipaddress import IPv4Address
from pathlib import Path
from shutil import rmtree
import sys
from tempfile import TemporaryDirectory
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from table_gen.engine.core.gen_types import PortMapping, RouteRecord  # noqa: E402
from table_gen.engine.core.mrt_filter import filter_mrt_routes  # noqa: E402
from table_gen.engine.utils.route_reader import read_port_map, read_routes  # noqa: E402
from table_gen.engine.utils.route_writer import FileFormat, save_port_map, save_routes  # noqa: E402

DEFAULT_OUTPUT_DIR = REPO_ROOT / "build/libs_cpp/lulea_verify/tables"


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


def synthetic_table(count: int, seed: int) -> list[RouteRecord]:
    """Use overlapping prefixes and cluster the remaining routes into L2 blocks."""
    base = int(IPv4Address("10.0.0.0")) + ((seed % 16) << 18)
    prefixes = [(0, 0), (base, 18), (base, 24), (base, 25), (base + 1, 32)][:count]
    for offset in range(1, count - len(prefixes) + 1):
        prefixes.append((base + (offset << 8), 24))
    return [
        RouteRecord(f"{IPv4Address(address)}/{mask}", mask, (seed + index) % 255 + 1)
        for index, (address, mask) in enumerate(prefixes)
    ]


def generate(
    source_file: Path | None,
    output_dir: Path,
    count: int,
    num_tables: int = 10,
    base_seed: int = 42,
) -> None:
    if count <= 0:
        raise ValueError("Route count must be positive")

    routes: list[RouteRecord] = []
    port_map: PortMapping | None = None
    if source_file is not None:
        print(f"Reading clean converted routes from: {source_file}")
        routes = read_routes(source_file)
        print(f"Total routes loaded: {len(routes)}")
        if count > len(routes):
            raise ValueError(f"Requested {count} routes, but the source contains only {len(routes)}")

        ports_candidate = source_file.with_name("routes.ports")
        if ports_candidate.is_file():
            port_map = read_port_map(ports_candidate)
            print(f"Loaded port map with {len(port_map)} ports")

    output_dir.mkdir(parents=True, exist_ok=True)
    for i in range(num_tables):
        idx = i + 1
        seed = base_seed + i * 137
        if source_file is None:
            sampled = synthetic_table(count, seed)
        else:
            sampled = filter_mrt_routes(routes, target_count=count, seed=seed)

        target_dir = output_dir / f"table_{idx}"
        meta = {
            "source_file": str(source_file) if source_file is not None else "synthetic",
            "seed": seed,
        }
        write_table_dir(target_dir, sampled, port_map, meta)
        unique_ports = len({r.port_id for r in sampled})
        print(f"  [{idx}/{num_tables}] -> {target_dir.name} ({len(sampled)} routes, {unique_ports} ports, seed={seed})")


def ninja_depfile_path(path: Path) -> str:
    """Escape a path in a Make-style dependency file."""
    return path.as_posix().replace("$", "$$").replace("#", r"\#").replace(" ", r"\ ")


def write_depfile(depfile: Path, stamp: Path, source: Path | None) -> None:
    """Track changes to the input table and its optional port map."""
    dependencies: list[Path] = []
    if source is not None:
        dependencies.append(source)
        ports = source.with_name("routes.ports")
        if ports.is_file():
            dependencies.append(ports)
    inputs = " ".join(ninja_depfile_path(path) for path in dependencies)
    depfile.write_text(f"{ninja_depfile_path(stamp)}: {inputs}\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__)
    parser.add_argument("-i", "--input", type=Path,
                        help="Converted route table (omit for synthetic routes)")
    parser.add_argument("-o", "--output-dir", type=Path,
                        default=DEFAULT_OUTPUT_DIR, help="Destination directory for tables")
    parser.add_argument("-c", "--count", type=int,
                        required=True, help="Number of routes per table")
    parser.add_argument("-n", "--num-tables", type=int,
                        default=10, help="Number of tables")
    parser.add_argument("-s", "--seed", type=int,
                        default=42, help="Base random seed")
    parser.add_argument("--stamp", type=Path, help="Replace the Meson fixture directory and mark it up to date")
    parser.add_argument("--depfile", type=Path, help="Write the fixture dependencies for Ninja")
    args = parser.parse_args()

    if args.input is not None and not args.input.is_file():
        print(f"Error: input file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    try:
        if args.stamp is None:
            generate(args.input, args.output_dir, args.count, args.num_tables, args.seed)
        else:
            with TemporaryDirectory(dir=args.output_dir.parent) as scratch:
                staging = Path(scratch) / "tables"
                generate(args.input, staging, args.count, args.num_tables, args.seed)
                if args.output_dir.is_dir():
                    rmtree(args.output_dir)
                staging.replace(args.output_dir)
            if args.depfile is not None:
                write_depfile(args.depfile, args.stamp, args.input)
            args.stamp.touch()
        print(f"\nAll {args.num_tables} tables generated in: {args.output_dir}")
    except ValueError as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    main()
