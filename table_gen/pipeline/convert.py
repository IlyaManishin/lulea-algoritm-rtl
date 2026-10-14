"""
Stage 1 logic: discover binary MRT archives and convert them to the
plain-text intermediate format.

This module contains no CLI or presentation code.
"""

import bz2
import gzip
import shutil
import tempfile
from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

from config import PORT_MAP_FILENAME, ROUTE_FILENAME
from core.mrt_reader import MRTParseError, parse_mrt_binary
from utils.port_mapping import map_next_hops_to_ports
from utils.route_saver import save_converted


# =========================================================================
# Data types
# =========================================================================

_ARCHIVE_SUFFIXES = (".bz2", ".gz")


@dataclass(frozen=True, slots=True)
class MrtFileEntry:
    """A discovered MRT file on disk."""
    path: Path
    rel_parent: Path   # relative directory from source root
    stem: str           # filename without archive suffix


class ConvertStatus(Enum):
    SUCCESS = auto()
    SKIPPED = auto()
    ERROR = auto()


@dataclass(frozen=True, slots=True)
class ConvertResult:
    """Outcome of converting a single MRT file."""
    entry: MrtFileEntry
    status: ConvertStatus
    route_count: int = 0
    port_count: int = 0
    error_message: str = ""


# =========================================================================
# MRT discovery
# =========================================================================

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
        entries.append(MrtFileEntry(path=candidate, rel_parent=rel_parent, stem=stem))
    return entries


# =========================================================================
# Decompression
# =========================================================================

def _decompress_to_tmp(src: Path) -> Path:
    """Decompress .bz2 / .gz into a named temporary file.  Caller must delete."""
    suffix = src.suffix.lower()
    tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".mrt")
    tmp_path = Path(tmp.name)
    tmp.close()

    if suffix == ".bz2":
        with bz2.open(src, "rb") as f_in, open(tmp_path, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)
    elif suffix == ".gz":
        with gzip.open(src, "rb") as f_in, open(tmp_path, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)
    else:
        raise ValueError(f"Unsupported archive format: {src.suffix}")

    return tmp_path


# =========================================================================
# Output path helpers
# =========================================================================

def output_dir_for(entry: MrtFileEntry, output_root: Path) -> Path:
    return output_root / entry.rel_parent / entry.stem


def _is_already_converted(out_dir: Path) -> bool:
    routes_file = out_dir / ROUTE_FILENAME
    ports_file = out_dir / PORT_MAP_FILENAME
    return (
        routes_file.exists()
        and routes_file.stat().st_size > 0
        and ports_file.exists()
        and ports_file.stat().st_size > 0
    )


# =========================================================================
# Single-file conversion
# =========================================================================

def convert_mrt_file(
    entry: MrtFileEntry,
    output_root: Path,
    *,
    force: bool = False,
) -> ConvertResult:
    """
    Parse a single binary MRT file and write the intermediate text format.

    Decompresses .bz2 / .gz into a temp file when necessary.
    Returns a ConvertResult describing the outcome.
    """
    out_dir = output_dir_for(entry, output_root)

    if not force and _is_already_converted(out_dir):
        return ConvertResult(entry=entry, status=ConvertStatus.SKIPPED)

    suffix = entry.path.suffix.lower()
    tmp_path: Path | None = None

    try:
        if suffix in _ARCHIVE_SUFFIXES:
            tmp_path = _decompress_to_tmp(entry.path)
            parse_target = tmp_path
        else:
            parse_target = entry.path

        routes = parse_mrt_binary(parse_target)

    except (OSError, MRTParseError, ValueError) as exc:
        return ConvertResult(
            entry=entry,
            status=ConvertStatus.ERROR,
            error_message=str(exc),
        )
    finally:
        if tmp_path is not None and tmp_path.exists():
            tmp_path.unlink()

    if not routes:
        return ConvertResult(
            entry=entry,
            status=ConvertStatus.ERROR,
            error_message="No routes found in file",
        )

    port_map = map_next_hops_to_ports(routes)

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
    )
