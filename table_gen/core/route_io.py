"""
Lightweight reader/writer for the intermediate plain-text route format.

File layout (routes.txt):
    <count>              # first line: integer number of records that follow
    <prefix> <next_hop>  # one record per line, e.g. "1.2.3.0/24 10.0.1.1"
    ...

File layout (routes.ports):
    <count>              # first line: integer number of port mappings that follow
    <next_hop> <port_id> # one mapping per line, e.g. "10.0.1.1 5"
    ...
"""

from pathlib import Path

from core.gen_types import RouteRecord


class RouteIOError(Exception):
    """Raised on malformed or unreadable route/port files."""
    pass


# =========================================================================
# Writers
# =========================================================================

def write_routes(
    routes: list[RouteRecord],
    output_path: str | Path,
) -> None:
    """Write routes in plain text format with a leading count line."""
    filepath = Path(output_path)
    filepath.parent.mkdir(parents=True, exist_ok=True)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(routes)}\n")
        for route in routes:
            f.write(f"{route.prefix} {route.next_hop}\n")


def write_port_map(
    port_map: dict[str, int],
    output_path: str | Path,
) -> None:
    """Write next-hop→port mapping with a leading count line."""
    filepath = Path(output_path)
    filepath.parent.mkdir(parents=True, exist_ok=True)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(port_map)}\n")
        for next_hop, port_id in port_map.items():
            f.write(f"{next_hop} {port_id}\n")


def write_routes_and_port_map(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    routes_path: str | Path,
    port_map_path: str | Path,
) -> None:
    write_routes(routes, routes_path)
    write_port_map(port_map, port_map_path)


# =========================================================================
# Readers
# =========================================================================

def read_routes(input_path: str | Path) -> list[RouteRecord]:
    """
    Read routes from a plain-text file produced by write_routes().

    Returns a list of RouteRecord.  The leading count line is validated
    against the actual number of records for early corruption detection.
    """
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"Routes file not found: {filepath}")

    routes: list[RouteRecord] = []

    try:
        with open(filepath, "r", encoding="utf-8") as f:
            count_line = f.readline().strip()
            if not count_line.isdigit():
                raise RouteIOError(
                    f"Expected integer count on first line, got: {count_line!r}"
                )
            expected_count = int(count_line)

            for lineno, line in enumerate(f, start=2):
                line = line.strip()
                if not line:
                    continue
                parts = line.split()
                if len(parts) != 2:
                    raise RouteIOError(
                        f"{filepath}:{lineno}: expected 'prefix next_hop', got: {line!r}"
                    )
                prefix, next_hop = parts
                try:
                    mask = int(prefix.split("/")[1])
                except (IndexError, ValueError):
                    raise RouteIOError(
                        f"{filepath}:{lineno}: cannot parse mask from prefix {prefix!r}"
                    )
                routes.append(RouteRecord(prefix=prefix, mask=mask, next_hop=next_hop))

    except (OSError, ValueError) as exc:
        raise RouteIOError(f"Failed to read routes file '{filepath}': {exc}") from exc

    if len(routes) != expected_count:
        raise RouteIOError(
            f"{filepath}: count header says {expected_count} records "
            f"but found {len(routes)}"
        )

    return routes


def read_route_count(input_path: str | Path) -> int:
    """
    Read only the count header from a routes file without loading all records.
    Useful for quick capacity checks before sampling.
    """
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"Routes file not found: {filepath}")

    with open(filepath, "r", encoding="utf-8") as f:
        count_line = f.readline().strip()

    if not count_line.isdigit():
        raise RouteIOError(
            f"Expected integer count on first line of {filepath}, got: {count_line!r}"
        )
    return int(count_line)
