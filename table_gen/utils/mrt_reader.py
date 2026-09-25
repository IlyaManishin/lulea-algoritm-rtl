from pathlib import Path
from enum import Enum, auto
from bgpkit import Parser

from gen_types import RouteRecord


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


def parse_mrt_binary(file_path: str | Path) -> list[RouteRecord]:
    unique_routes: dict[str, RouteRecord] = {}

    parser = Parser(str(file_path))
    for elem in parser:
        prefix = elem.prefix
        next_hop = elem.next_hop

        if prefix and next_hop and prefix not in unique_routes:
            if '/' in prefix:
                mask = int(prefix.split('/')[1])
                unique_routes[prefix] = RouteRecord(
                    prefix=prefix,
                    mask=mask,
                    next_hop=next_hop
                )

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