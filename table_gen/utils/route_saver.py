"""Serialisation helpers for route tables and port maps (text and binary)."""

import socket
import struct
from enum import Enum, auto
from pathlib import Path

from core.gen_types import RouteRecord

MAGIC_ROUTE_BINARY = b"LRB1"
MAGIC_PORT_BINARY = b"LPB1"

_ROUTE_PACKER = struct.Struct(">IBH")
_PORT_PACKER = struct.Struct(">HI")


class FileFormat(Enum):
    TEXT = auto()
    BINARY = auto()


def _ensure_parent(filepath: Path) -> None:
    filepath.parent.mkdir(parents=True, exist_ok=True)


def _normalize_port_map(port_map: dict[str, int] | dict[int, str]) -> dict[int, str]:
    normalized: dict[int, str] = {}
    for k, v in port_map.items():
        if isinstance(k, int):
            normalized[k] = v
        else:
            normalized[v] = k
    return normalized


def save_port_map(
    port_map: dict[str, int] | dict[int, str],
    output_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Write port mapping in text or binary format."""
    filepath = Path(output_path)
    _ensure_parent(filepath)
    normalized = _normalize_port_map(port_map)

    if file_format == FileFormat.BINARY:
        buf = bytearray(8 + len(normalized) * 6)
        buf[:8] = MAGIC_PORT_BINARY + struct.pack(">I", len(normalized))
        offset = 8
        for port_id, next_hop in normalized.items():
            ip_int = struct.unpack(">I", socket.inet_aton(next_hop))[0]
            _PORT_PACKER.pack_into(buf, offset, port_id, ip_int)
            offset += 6
        with open(filepath, "wb") as f:
            f.write(buf)
    else:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(f"{len(normalized)}\n")
            for port_id, next_hop in normalized.items():
                f.write(f"{port_id} {next_hop}\n")


def save_routes(
    routes: list[RouteRecord],
    output_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Write route table in text or binary format."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    if file_format == FileFormat.BINARY:
        buf = bytearray(8 + len(routes) * 8)
        buf[:8] = MAGIC_ROUTE_BINARY + struct.pack(">I", len(routes))
        offset = 8
        for route in routes:
            _ROUTE_PACKER.pack_into(buf, offset, route.ip_int, route.mask, route.port_id)
            offset += 8
        with open(filepath, "wb") as f:
            f.write(buf)
    else:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(f"{len(routes)}\n")
            for route in routes:
                f.write(f"{route.prefix} {route.port_id}\n")


def save_route_table(
    routes: list[RouteRecord],
    port_map: dict[str, int] | None,
    output_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Write compiled route table in text or binary format."""
    save_routes(routes, output_path, file_format=file_format)


def save_routes_and_ports(
    routes: list[RouteRecord],
    port_map: dict[str, int] | dict[int, str],
    routes_output_path: str | Path,
    port_map_output_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Save route table + port map side by side in specified format."""
    save_port_map(port_map, port_map_output_path, file_format=file_format)
    save_routes(routes, routes_output_path, file_format=file_format)


def save_converted(
    routes: list[RouteRecord],
    port_map: dict[str, int] | dict[int, str],
    routes_path: str | Path,
    port_map_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Save converted route table and port map in specified format."""
    save_routes(routes, routes_path, file_format=file_format)
    save_port_map(port_map, port_map_path, file_format=file_format)
