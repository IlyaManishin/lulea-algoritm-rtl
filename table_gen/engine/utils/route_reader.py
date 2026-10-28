"""Parsing helpers for route tables and port maps with binary/text auto-detection."""

import socket
import struct
from pathlib import Path
from typing import BinaryIO

from ..config import (
    MAGIC_PORT_BIN,
    MAGIC_ROUTE_BIN,
    PORT_RECORD_SIZE,
    PORT_STRUCT_FORMAT,
    ROUTE_RECORD_SIZE,
    ROUTE_STRUCT_FORMAT,
    ROUTE_COUNT_FORMAT
)
from ..core.gen_types import PortMapping, RouteRecord

_ROUTE_UNPACKER = struct.Struct(ROUTE_STRUCT_FORMAT)
_PORT_UNPACKER = struct.Struct(PORT_STRUCT_FORMAT)


class RouteReadError(Exception):
    """Raised on malformed or unreadable route/port files."""
    pass


def read_route_count(input_path: str | Path) -> int:
    """Read record count from text or binary file in O(1)."""
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"File not found: {filepath}")

    with open(filepath, "rb") as f:
        magic = f.read(4)
        if magic in (MAGIC_ROUTE_BIN, MAGIC_PORT_BIN):
            count_bytes = f.read(4)
            if len(count_bytes) < 4:
                raise RouteReadError(f"Truncated binary header in {filepath}")
            return struct.unpack("<I", count_bytes)[0]

        f.seek(0)
        first_line = f.readline().decode("utf-8", errors="replace").strip()

    if not first_line.isdigit():
        raise RouteReadError(
            f"Expected integer count on first line of {filepath}, got: {first_line!r}"
        )
    return int(first_line)


def _read_routes_binary(f: BinaryIO, expected_count: int, filepath: Path) -> list[RouteRecord]:
    body = f.read()
    if len(body) != expected_count * ROUTE_RECORD_SIZE:
        raise RouteReadError(
            f"{filepath}: binary payload size mismatch (expected {expected_count * ROUTE_RECORD_SIZE} bytes, got {len(body)})"
        )

    routes: list[RouteRecord] = []
    for ip_int, mask, port_id in _ROUTE_UNPACKER.iter_unpack(body):
        ip_str = socket.inet_ntoa(struct.pack(">I", ip_int))
        routes.append(RouteRecord(
            prefix=f"{ip_str}/{mask}", mask=mask, port_id=port_id))
    return routes


def _read_routes_text(f: BinaryIO, filepath: Path) -> list[RouteRecord]:
    first_line = f.readline().decode("utf-8", errors="replace").strip()
    if not first_line.isdigit():
        raise RouteReadError(
            f"Expected integer count on first line of {filepath}, got: {first_line!r}"
        )
    expected_count = int(first_line)

    routes: list[RouteRecord] = []
    for lineno, raw_line in enumerate(f, start=2):
        line = raw_line.decode("utf-8", errors="replace").strip()
        if not line:
            continue

        parts = line.split()
        if len(parts) != 2:
            raise RouteReadError(
                f"{filepath}:{lineno}: expected 'prefix port_id', got: {line!r}"
            )

        prefix, port_str = parts
        try:
            mask = int(prefix.split("/")[1])
        except (IndexError, ValueError) as exc:
            raise RouteReadError(
                f"{filepath}:{lineno}: cannot parse mask from prefix {prefix!r}"
            ) from exc

        if not port_str.isdigit():
            raise RouteReadError(
                f"{filepath}:{lineno}: port_id must be integer, got: {port_str!r}"
            )

        routes.append(RouteRecord(
            prefix=prefix, mask=mask, port_id=int(port_str)))

    if len(routes) != expected_count:
        raise RouteReadError(
            f"{filepath}: count header says {expected_count} records but found {len(routes)}"
        )

    return routes


def read_routes(input_path: str | Path) -> list[RouteRecord]:
    """Read routes with automatic format detection (binary vs text)."""
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"Routes file not found: {filepath}")

    with open(filepath, "rb") as f:
        magic = f.read(4)
        if magic == MAGIC_ROUTE_BIN:
            count_bytes = f.read(4)
            if len(count_bytes) < 4:
                raise RouteReadError(f"Truncated binary header in {filepath}")
            expected_count = struct.unpack(ROUTE_COUNT_FORMAT, count_bytes)[0]
            return _read_routes_binary(f, expected_count, filepath)
        else:
            f.seek(0)
            return _read_routes_text(f, filepath)


def _read_port_map_binary(f: BinaryIO, expected_count: int, filepath: Path) -> PortMapping:
    body = f.read()
    if len(body) != expected_count * PORT_RECORD_SIZE:
        raise RouteReadError(
            f"{filepath}: binary port payload size mismatch (expected {expected_count * PORT_RECORD_SIZE} bytes, got {len(body)})"
        )

    port_map: PortMapping = {}
    for port_id, ip_int in _PORT_UNPACKER.iter_unpack(body):
        port_map[port_id] = socket.inet_ntoa(struct.pack(">I", ip_int))
    return port_map


def _read_port_map_text(f: BinaryIO, filepath: Path) -> PortMapping:
    first_line = f.readline().decode("utf-8", errors="replace").strip()
    if not first_line.isdigit():
        raise RouteReadError(
            f"Expected integer count on first line of {filepath}, got: {first_line!r}"
        )
    expected_count = int(first_line)

    port_map: PortMapping = {}
    for lineno, raw_line in enumerate(f, start=2):
        line = raw_line.decode("utf-8", errors="replace").strip()
        if not line:
            continue

        parts = line.split()
        if len(parts) != 2:
            raise RouteReadError(
                f"{filepath}:{lineno}: expected 'port_id next_hop', got: {line!r}"
            )

        port_str, next_hop = parts
        if not port_str.isdigit():
            raise RouteReadError(
                f"{filepath}:{lineno}: port_id must be integer, got: {port_str!r}"
            )
        port_map[int(port_str)] = next_hop

    if len(port_map) != expected_count:
        raise RouteReadError(
            f"{filepath}: count header says {expected_count} ports but found {len(port_map)}"
        )

    return port_map


def read_port_map(input_path: str | Path) -> PortMapping:
    """Read port map with automatic format detection returning {port_id: next_hop}."""
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"Port map file not found: {filepath}")

    with open(filepath, "rb") as f:
        magic = f.read(4)
        if magic == MAGIC_PORT_BIN:
            count_bytes = f.read(4)
            if len(count_bytes) < 4:
                raise RouteReadError(f"Truncated binary header in {filepath}")
            expected_count = struct.unpack(ROUTE_COUNT_FORMAT, count_bytes)[0]
            return _read_port_map_binary(f, expected_count, filepath)
        else:
            f.seek(0)
            return _read_port_map_text(f, filepath)
