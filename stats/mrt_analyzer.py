"""Summarize routing tables, MRT dumps, and port maps by file contents."""

import argparse
from collections import Counter
from collections.abc import Iterable, Iterator
from dataclasses import dataclass, field
from enum import Enum, auto
from ipaddress import ip_address, ip_network
from pathlib import Path
import re
from struct import Struct
import sys
from tempfile import TemporaryDirectory

try:
    if not __package__:
        sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from table_gen.engine.config import MAGIC_PORT_BIN, MAGIC_ROUTE_BIN
    from table_gen.engine.core.mrt_reader import (
        MRTParseError, ReaderResponse, iter_mrt_text_records, parse_mrt_binary,
    )
    from table_gen.engine.utils.route_reader import RouteReadError, read_port_map, read_routes
except ModuleNotFoundError as error:
    if __name__ != "__main__":
        raise
    raise SystemExit(f"{error}. Run with the project's Python environment.") from error

# RFC 6396 common header and the types that carry BGP routing data.
MRT_HEADER = Struct(">IHHI")
MRT_BGP_TYPES = {12, 13, 16, 17}
MRT_COMPRESSION = {b"\x1f\x8b": ".gz", b"BZh": ".bz2"}


class InputFormat(Enum):
    """Formats accepted by the analyzer."""

    ROUTES = auto()
    PORT_MAP = auto()
    ROUTE_PAIRS = auto()
    MRT_TEXT = auto()
    MRT_BINARY = auto()


def detect_format(path: Path) -> InputFormat:
    """Identify the input without relying on its suffix."""
    with path.open("rb") as stream:
        head = stream.read(4096)
    if head.startswith(MAGIC_ROUTE_BIN):
        return InputFormat.ROUTES
    if head.startswith(MAGIC_PORT_BIN):
        return InputFormat.PORT_MAP
    if head.startswith(tuple(MRT_COMPRESSION)):
        return InputFormat.MRT_BINARY
    if len(head) >= MRT_HEADER.size:
        _, record_type, _, length = MRT_HEADER.unpack_from(head)
        if record_type in MRT_BGP_TYPES and 0 < length <= path.stat().st_size - MRT_HEADER.size:
            return InputFormat.MRT_BINARY

    return detect_text_format(head, path)


def detect_text_format(head: bytes, path: Path) -> InputFormat:
    """Distinguish counted tables, prefix pairs, and bgpdump records."""
    lines = [line.strip() for line in head.splitlines() if line.strip()]
    if lines:
        first = lines[0]
        if first.isdigit():
            has_port_rows = len(lines) > 1 and lines[1].split()[0].isdigit()
            empty_port_map = first == b"0" and len(lines) == 1 and path.suffix == ".ports"
            if has_port_rows or empty_port_map:
                return InputFormat.PORT_MAP
            return InputFormat.ROUTES
        if re.match(rb"^[A-Z][A-Z0-9_]*:", first):
            return InputFormat.MRT_TEXT
        if b"/" in first.split()[0]:
            return InputFormat.ROUTE_PAIRS

    raise ValueError(f"Unrecognized routing input: {path}")


def read_binary_mrt(path: Path) -> ReaderResponse:
    """Pass an appropriate suffix to the MRT reader without copying the dump."""
    with path.open("rb") as stream:
        head = stream.read(3)
    suffix = ".mrt"
    for magic, extension in MRT_COMPRESSION.items():
        if head.startswith(magic):
            suffix = extension
            break
    if path.suffix == suffix:
        return parse_mrt_binary(path)

    with TemporaryDirectory() as temporary_dir:
        alias = Path(temporary_dir) / f"dump{suffix}"
        alias.symlink_to(path.resolve(strict=True))
        return parse_mrt_binary(alias)


@dataclass(slots=True)
class RouteStats:
    """Prefix counts and egress values for the unique routes in a table."""

    prefixes: set[str] = field(default_factory=set)
    masks: Counter[int] = field(default_factory=Counter)
    egress: set[int | str] = field(default_factory=set)

    def add(self, prefix: str, mask: int | None, egress: int | str | None) -> bool:
        """Count the first route for a prefix and report whether it was new."""
        if prefix in self.prefixes:
            return False
        self.prefixes.add(prefix)
        if mask is not None:
            self.masks[mask] += 1
        if egress is not None:
            self.egress.add(egress)
        return True

    def print_report(self, egress_name: str) -> None:
        """Print the unique prefix count, egress summary, and mask distribution."""
        print(f"Unique prefixes: {len(self.prefixes):,}")
        print(f"Unique {egress_name}: {len(self.egress):,}")
        print(f"First {egress_name}: {', '.join(sorted(map(str, self.egress))[:10]) or 'none'}")
        print(f"\n{'Mask':<8} {'Count':>12} {'Share':>9}")
        total_masks = self.masks.total()
        for mask, count in sorted(self.masks.items()):
            share = count / total_masks
            print(f"/{mask:<7} {count:>12,} {share:>9.2%}")


def summarize(routes: Iterable[tuple[str, int, int | str]], egress_name: str) -> None:
    """Print common route statistics from parsed records."""
    stats = RouteStats()
    for prefix, mask, egress in routes:
        stats.add(prefix, mask, egress)
    stats.print_report(egress_name)


def read_route_pairs(path: Path) -> Iterator[tuple[str, int, int | str]]:
    """Read headerless prefix and egress pairs, including MRT sample exports."""
    with path.open(encoding="utf-8") as stream:
        for number, line in enumerate(stream, start=1):
            if not line.strip():
                continue
            try:
                prefix, egress = line.split()
                network = ip_network(prefix, strict=False)
                value = int(egress) if egress.isdecimal() else str(ip_address(egress))
            except ValueError as error:
                raise ValueError(f"{path}:{number}: invalid prefix and egress pair") from error
            yield str(network), network.prefixlen, value


def analyze_mrt_text(path: Path) -> None:
    """Report routes and BGP metadata from a text dump."""
    stats = RouteStats()
    neighbors: set[str] = set()
    origins: set[str] = set()
    for record in iter_mrt_text_records(path, encoding_errors="ignore"):
        prefix = record.get("PREFIX")
        if prefix is None:
            continue
        try:
            mask = int(prefix.split("/", 2)[1])
        except (IndexError, ValueError):
            mask = None
        if not stats.add(prefix, mask, record.get("NEXT_HOP")):
            continue
        peer = re.search(r"AS(\d+)", record.get("FROM", ""))
        if peer:
            neighbors.add(peer.group(1))
        as_path = record.get("ASPATH", "").split()
        if as_path:
            origins.add(as_path[-1])
    if not stats.prefixes:
        raise ValueError(f"No prefix records found in {path}")
    print(f"Unique BGP neighbor ASes: {len(neighbors):,}")
    print(f"Unique origin ASes: {len(origins):,}")
    stats.print_report("next hops")


def analyze(path: Path) -> None:
    """Dispatch to the project reader for the detected format."""
    file_format = detect_format(path)
    print(f"Input format: {file_format.name.lower().replace('_', ' ')}")
    match file_format:
        case InputFormat.ROUTES:
            routes = read_routes(path)
            summarize(((r.prefix, r.mask, r.port_id) for r in routes), "port IDs")
        case InputFormat.PORT_MAP:
            ports = read_port_map(path)
            print(f"Port IDs: {len(ports):,}")
            print(f"Unique next hops: {len(set(ports.values())):,}")
            for port, next_hop in sorted(ports.items())[:10]:
                print(f"  {port}: {next_hop}")
        case InputFormat.ROUTE_PAIRS:
            summarize(read_route_pairs(path), "egress values")
        case InputFormat.MRT_TEXT:
            analyze_mrt_text(path)
        case InputFormat.MRT_BINARY:
            result = read_binary_mrt(path)
            print(f"IPv4 MRT entries read: {result.total_elems:,}")
            if not result.routes:
                raise ValueError(f"No IPv4 routes found in {path}")
            summarize(((r.prefix, r.mask, r.next_hop) for r in result.routes), "next hops")


def main() -> None:
    """Analyze a file supplied by the caller or routes.txt in the current directory."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "path", type=Path, nargs="?", default=Path("routes.txt"),
        help="text routes, LRB1 routes, a port map, or an MRT dump (plain, gzip, bzip2)",
    )
    args = parser.parse_args()
    try:
        analyze(args.path)
    except (OSError, ValueError, MRTParseError, RouteReadError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")


if __name__ == "__main__":
    main()
