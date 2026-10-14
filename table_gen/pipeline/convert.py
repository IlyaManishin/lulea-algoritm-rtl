"""Stage 1: discover binary MRT archives and convert to intermediate route tables."""

from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

from config import INFO_FILENAME, PORT_MAP_FILENAME, ROUTE_FILENAME
from core.gen_types import MrtRecord, RouteRecord
from core.mrt_reader import MRTParseError, ReaderResponse, parse_mrt_binary
from utils.port_mapping import map_next_hops_to_ports
from utils.route_saver import save_converted

_ARCHIVE_SUFFIXES = (".bz2", ".gz")


@dataclass(frozen=True, slots=True)
class MrtFileEntry:
    path: Path
    rel_parent: Path
    stem: str


class ConvertStatus(Enum):
    SUCCESS = auto()
    SKIPPED = auto()
    ERROR = auto()


@dataclass(frozen=True, slots=True)
class ConvertResult:
    entry: MrtFileEntry
    status: ConvertStatus
    route_count: int = 0
    port_count: int = 0
    total_elements: int = 0
    filtered_ipv6_count: int = 0
    error_message: str = ""


def _strip_archive_suffix(name: str) -> str:
    for suffix in _ARCHIVE_SUFFIXES:
        if name.endswith(suffix):
            return name[: -len(suffix)]
    return name


def discover_mrt_files(source_root: Path) -> list[MrtFileEntry]:
    """Walk source_root and return all MRT file entries."""
    entries: list[MrtFileEntry] = []
    for candidate in sorted(source_root.rglob("*")):
        if not candidate.is_file():
            continue
        rel_parent = candidate.parent.relative_to(source_root)
        stem = _strip_archive_suffix(candidate.name)
        entries.append(MrtFileEntry(
            path=candidate, rel_parent=rel_parent, stem=stem))
    return entries


def output_dir_for(entry: MrtFileEntry, output_root: Path) -> Path:
    return output_root / entry.rel_parent / entry.stem


def _is_already_converted(out_dir: Path) -> bool:
    routes_file = out_dir / ROUTE_FILENAME
    ports_file = out_dir / PORT_MAP_FILENAME
    info_file = out_dir / INFO_FILENAME
    return (
        routes_file.exists()
        and routes_file.stat().st_size > 0
        and ports_file.exists()
        and ports_file.stat().st_size > 0
        and info_file.exists()
        and info_file.stat().st_size > 0
    )


def _save_convert_info(info_path: Path, source_file: str,
                       reader_resp: ReaderResponse,
                       port_map: dict[int, str],
                       routes: list[RouteRecord]):
    with open(info_path, "w", encoding="utf-8") as f:
        f.write(f"source_file: {source_file}\n")
        f.write(f"total_elements: {reader_resp.total_elems}\n")
        f.write(f"unique_routes: {len(routes)}\n")
        f.write(f"unique_ports: {len(port_map)}\n")
        f.write(f"filtered_ipv6: {reader_resp.filtered_ipv6_count}\n")
        f.write(f"skipped_elements: {reader_resp.skipped_elems}\n")


def convert_mrt_file(
    entry: MrtFileEntry,
    output_root: Path,
    *,
    force: bool = False,
) -> ConvertResult:
    """
    Parse a binary MRT archive directly via streaming and write converted files.

    Streaming reads .bz2 and .gz directly without temp files on disk/RAM.
    """
    out_dir = output_dir_for(entry, output_root)

    if not force and _is_already_converted(out_dir):
        return ConvertResult(entry=entry, status=ConvertStatus.SKIPPED)

    try:
        reader_resp: ReaderResponse = parse_mrt_binary(entry.path)
    except (OSError, MRTParseError, ValueError) as exc:
        return ConvertResult(
            entry=entry,
            status=ConvertStatus.ERROR,
            error_message=str(exc),
        )

    mrt_records: list[MrtRecord] = reader_resp.routes
    if not mrt_records:
        return ConvertResult(
            entry=entry,
            status=ConvertStatus.ERROR,
            error_message="No valid IPv4 routes found in file",
        )

    port_map: dict[str, int] = map_next_hops_to_ports(mrt_records)
    routes: list[RouteRecord] = [
        RouteRecord(prefix=m.prefix, mask=m.mask, port_id=port_map[m.next_hop])
        for m in mrt_records
    ]

    total_elements = reader_resp.total_elems
    filtered_ipv6_count = reader_resp.filtered_ipv6_count

    info_path = out_dir / INFO_FILENAME
    _save_convert_info(info_path, entry.path.name, reader_resp, port_map, routes)

    del reader_resp
    del mrt_records

    save_converted(
        routes=routes,
        port_map=port_map,
        routes_path=out_dir / ROUTE_FILENAME,
        port_map_path=out_dir / PORT_MAP_FILENAME,
    )

    return ConvertResult(
        entry=entry,
        status=ConvertStatus.SUCCESS,
        route_count=len(routes),
        port_count=len(port_map),
        total_elements=total_elements,
        filtered_ipv6_count=filtered_ipv6_count,
    )
