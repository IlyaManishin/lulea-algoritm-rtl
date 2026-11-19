"""Serialisation helpers for route tables and port maps (text and binary)."""

import os
import socket
import struct
from enum import Enum, auto
from pathlib import Path

from ..config import (
    BIN_HEADER_SIZE,
    MAGIC_PORT_BIN,
    MAGIC_ROUTE_BIN,
    PORT_RECORD_SIZE,
    PORT_STRUCT_FORMAT,
    ROUTE_RECORD_SIZE,
    ROUTE_STRUCT_FORMAT,
    ROUTE_COUNT_FORMAT
)
from ..core.gen_types import PortMapping, RouteRecord

_ROUTE_PACKER = struct.Struct(ROUTE_STRUCT_FORMAT)
_PORT_PACKER = struct.Struct(PORT_STRUCT_FORMAT)


class FileFormat(Enum):
    TEXT = auto()
    BINARY = auto()


def _ensure_parent(filepath: Path) -> None:
    filepath.parent.mkdir(parents=True, exist_ok=True)

def _next_hop_to_int(next_hop: str):
    return struct.unpack(">I", socket.inet_aton(next_hop))[0]

# Atomic writes: write to temporary file first, then atomically replace.
# Guarantees files are never left half-written or corrupted if interrupted.
def _atomic_write_bytes(filepath: Path, data: bytes | bytearray) -> None:
    temp_path = filepath.with_name(f".{filepath.name}.tmp.{os.getpid()}")
    try:
        with open(temp_path, "wb") as f:
            f.write(data)
        os.replace(temp_path, filepath)
    except Exception:
        if temp_path.exists():
            temp_path.unlink()
        raise


def _atomic_write_text(filepath: Path, content: str) -> None:
    temp_path = filepath.with_name(f".{filepath.name}.tmp.{os.getpid()}")
    try:
        with open(temp_path, "w", encoding="utf-8") as f:
            f.write(content)
        os.replace(temp_path, filepath)
    except Exception:
        if temp_path.exists():
            temp_path.unlink()
        raise


def save_port_map(
    port_map: PortMapping,
    output_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Write port mapping in text or binary format."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    if file_format == FileFormat.BINARY:
        buf = bytearray(BIN_HEADER_SIZE + len(port_map) * PORT_RECORD_SIZE)
        buf[:BIN_HEADER_SIZE] = MAGIC_PORT_BIN + struct.pack(ROUTE_COUNT_FORMAT, len(port_map))
        offset = BIN_HEADER_SIZE
        
        for port_id, next_hop in port_map.items():
            ip_int = _next_hop_to_int(next_hop)
            _PORT_PACKER.pack_into(buf, offset, port_id, ip_int)
            offset += PORT_RECORD_SIZE
            
        _atomic_write_bytes(filepath, buf)
    else:
        lines = [f"{len(port_map)}\n"]
        for port_id, next_hop in port_map.items():
            lines.append(f"{port_id} {next_hop}\n")
        _atomic_write_text(filepath, "".join(lines))


def save_routes(
    routes: list[RouteRecord],
    output_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Write route table in text or binary format."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    if file_format == FileFormat.BINARY:
        buf = bytearray(BIN_HEADER_SIZE + len(routes) * ROUTE_RECORD_SIZE)
        buf[:BIN_HEADER_SIZE] = MAGIC_ROUTE_BIN + struct.pack(ROUTE_COUNT_FORMAT, len(routes))
        offset = BIN_HEADER_SIZE
        for route in routes:
            _ROUTE_PACKER.pack_into(
                buf, offset, route.ip_int, route.mask, route.port_id)
            offset += ROUTE_RECORD_SIZE
        _atomic_write_bytes(filepath, buf)
    else:
        lines = [f"{len(routes)}\n"]
        for route in routes:
            lines.append(f"{route.prefix} {route.port_id}\n")
        _atomic_write_text(filepath, "".join(lines))


def save_routes_and_ports(
    routes: list[RouteRecord],
    port_map: PortMapping,
    routes_path: str | Path,
    port_map_path: str | Path,
    file_format: FileFormat = FileFormat.TEXT,
) -> None:
    """Save route table and port map in specified format."""
    save_routes(routes, routes_path, file_format=file_format)
    save_port_map(port_map, port_map_path, file_format=file_format)
