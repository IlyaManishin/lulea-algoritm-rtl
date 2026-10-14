from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path
from bgpkit import Parser

from .gen_types import RouteRecord


@dataclass(slots=True, frozen=True)
class ReaderResponse:
    routes: list[RouteRecord]
    total_elems: int
    unique_routes_count: int
    skipped_elems: int
    filtered_ipv6_count: int


class MRTFileType(Enum):
    TEXT = auto()
    BINARY = auto()


class MRTParseError(Exception):
    """Raised when parsing an MRT file fails due to format or read errors."""
    pass


def _process_text_record(
    record: dict[str, str],
    unique_routes: dict[str, RouteRecord]
) -> None:
    prefix = record.get('PREFIX')
    next_hop = record.get('NEXT_HOP')

    if prefix and next_hop and prefix not in unique_routes:
        if '/' in prefix:
            try:
                mask = int(prefix.split('/')[1])
                unique_routes[prefix] = RouteRecord(
                    prefix=prefix,
                    mask=mask,
                    next_hop=next_hop
                )
            except (ValueError, IndexError):
                return


def parse_mrt_text(file_path: str | Path) -> list[RouteRecord]:
    path = Path(file_path)
    if not path.is_file():
        raise FileNotFoundError(f"MRT text file not found: {path}")

    unique_routes: dict[str, RouteRecord] = {}
    current_record: dict[str, str] = {}

    try:
        with open(path, 'r', encoding='utf-8') as f:
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
    except (OSError, ValueError) as e:
        raise MRTParseError(f"Failed to read MRT text file '{path}': {e}") from e

    return list(unique_routes.values())


def parse_mrt_binary(file_path: str | Path) -> ReaderResponse:
    path = Path(file_path)
    if not path.is_file():
        raise FileNotFoundError(f"MRT binary file not found: {path}")

    unique_routes: dict[str, RouteRecord] = {}
    total_elems = 0
    filtered_ipv6_count = 0
    skipped_elems = 0

    try:
        parser = Parser(str(path), filters={"ip_version": "ipv4"})
        for elem in parser:
            total_elems += 1
            prefix: str = elem.prefix
            next_hop: str = elem.next_hop

            if not prefix or not next_hop:
                skipped_elems += 1
                continue

            if ":" in prefix or "." not in prefix:
                filtered_ipv6_count += 1
                skipped_elems += 1
                continue

            if "/" not in prefix:
                skipped_elems += 1
                continue

            try:
                mask = int(prefix.split("/")[1])
                if mask < 1 or mask > 32:
                    skipped_elems += 1
                    continue
            except (ValueError, IndexError):
                skipped_elems += 1
                continue

            if prefix not in unique_routes:
                unique_routes[prefix] = RouteRecord(
                    prefix=prefix,
                    mask=mask,
                    next_hop=next_hop,
                )
            else:
                skipped_elems += 1

    except Exception as e:
        raise MRTParseError(f"Failed to parse binary MRT file '{path}': {e}") from e

    return ReaderResponse(
        routes=list(unique_routes.values()),
        total_elems=total_elems,
        unique_routes_count=len(unique_routes),
        skipped_elems=skipped_elems,
        filtered_ipv6_count=filtered_ipv6_count,
    )


def read_mrt_routes(
    file_path: str | Path,
    file_type: MRTFileType = MRTFileType.TEXT
) -> list[RouteRecord]:
    if file_type == MRTFileType.TEXT:
        return parse_mrt_text(file_path)
    elif file_type == MRTFileType.BINARY:
        return parse_mrt_binary(file_path).routes
    else:
        raise ValueError(f"Unsupported MRT file type: {file_type}")