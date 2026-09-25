import struct
import socket
from pathlib import Path
from enum import Enum, auto

from ..gen_types import RouteRecord


class MRTFileType(Enum):
    TEXT = auto()
    BINARY = auto()


def _process_text_record(
    record: dict[str, str],
    unique_routes: dict[str, RouteRecord]
) -> None:
    prefix = record.get('PREFIX')
    next_hop = record.get('NEXT_HOP')

    if prefix and next_hop and prefix not in unique_routes:
        if '/' in prefix:
            mask = int(prefix.split('/')[1])
            unique_routes[prefix] = RouteRecord(
                prefix=prefix,
                mask=mask,
                next_hop=next_hop
            )


def parse_mrt_text(file_path: str | Path) -> list[RouteRecord]:
    unique_routes: dict[str, RouteRecord] = {}
    current_record: dict[str, str] = {}

    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.strip()

            if not line:
                if current_record:
                    _process_text_record(current_record, unique_routes)
                    current_record = {}
                continue

            if ':' in line:
                key, val = line.split(':', 1)
                current_record[key.strip()] = val.strip()

        if current_record:
            _process_text_record(current_record, unique_routes)

    return list(unique_routes.values())


def _parse_peer_index_table(data: bytes) -> dict[int, str]:
    peer_entries: dict[int, str] = {}
    offset = 4
    view_len = struct.unpack('>H', data[2:4])[0]
    offset += view_len
    peer_count = struct.unpack('>H', data[offset:offset + 2])[0]
    offset += 2

    for i in range(peer_count):
        peer_type = data[offset]
        offset += 5

        if peer_type & 0x01:  # IPv6
            peer_ip = socket.inet_ntop(
                socket.AF_INET6, data[offset:offset + 16])
            offset += 16
        else:  # IPv4
            peer_ip = socket.inet_ntop(
                socket.AF_INET, data[offset:offset + 4])
            offset += 4

        offset += 4 if (peer_type & 0x02) else 2
        peer_entries[i] = peer_ip

    return peer_entries


def _extract_next_hop_from_attrs(attr_data: bytes) -> str | None:
    a_offset = 0
    while a_offset < len(attr_data):
        flags = attr_data[a_offset]
        type_code = attr_data[a_offset + 1]
        a_offset += 2

        if flags & 0x10:
            a_len = struct.unpack('>H', attr_data[a_offset:a_offset + 2])[0]
            a_offset += 2
        else:
            a_len = attr_data[a_offset]
            a_offset += 1

        if type_code == 3 and a_len == 4:
            return socket.inet_ntop(
                socket.AF_INET, attr_data[a_offset:a_offset + 4])

        a_offset += a_len

    return None


def _parse_rib_record(
    data: bytes,
    mrt_subtype: int,
    peer_entries: dict[int, str],
    unique_routes: dict[str, RouteRecord]
) -> None:
    offset = 4
    mask = data[offset]
    offset += 1

    prefix_bytes_len = (mask + 7) // 8
    prefix_raw = data[offset:offset + prefix_bytes_len] + \
        b'\x00' * (4 - prefix_bytes_len)
    offset += prefix_bytes_len
    prefix_ip = socket.inet_ntop(socket.AF_INET, prefix_raw)
    prefix = f"{prefix_ip}/{mask}"

    entry_count = struct.unpack('>H', data[offset:offset + 2])[0]
    offset += 2

    for _ in range(entry_count):
        if offset >= len(data):
            break

        peer_idx = struct.unpack('>H', data[offset:offset + 2])[0]
        offset += 6
        if mrt_subtype == 4:
            offset += 4

        attr_len = struct.unpack('>H', data[offset:offset + 2])[0]
        offset += 2
        attr_data = data[offset:offset + attr_len]
        offset += attr_len

        next_hop = _extract_next_hop_from_attrs(attr_data)

        if not next_hop and peer_idx in peer_entries:
            next_hop = peer_entries[peer_idx]

        if prefix and next_hop and prefix not in unique_routes:
            unique_routes[prefix] = RouteRecord(
                prefix=prefix,
                mask=mask,
                next_hop=next_hop
            )


def parse_mrt_binary(file_path: str | Path) -> list[RouteRecord]:
    unique_routes: dict[str, RouteRecord] = {}
    peer_entries: dict[int, str] = {}

    with open(file_path, 'rb') as f:
        while True:
            header = f.read(12)
            if len(header) < 12:
                break

            _, mrt_type, mrt_subtype, length = struct.unpack('>IHHI', header)
            data = f.read(length)
            if len(data) < length:
                break

            if mrt_type == 13:  # TABLE_DUMP_V2
                if mrt_subtype == 1:  # PEER_INDEX_TABLE
                    peer_entries = _parse_peer_index_table(data)
                elif mrt_subtype in (2, 4):  # RIB_IPV4_UNICAST / ADDPATH
                    _parse_rib_record(
                        data, mrt_subtype, peer_entries, unique_routes)

    return list(unique_routes.values())


def read_mrt_routes(
    file_path: str | Path,
    file_type: MRTFileType = MRTFileType.TEXT
) -> list[RouteRecord]:
    if file_type == MRTFileType.TEXT:
        return parse_mrt_text(file_path)
    elif file_type == MRTFileType.BINARY:
        return parse_mrt_binary(file_path)
    else:
        raise ValueError(f"Unsupported MRT file type: {file_type}")