"""Parsing helpers for route tables and port maps."""

from pathlib import Path

from core.gen_types import RouteRecord


class RouteReadError(Exception):
    """Raised on malformed or unreadable route files."""
    pass


def read_route_count(input_path: str | Path) -> int:
    """Read only the count header without loading records."""
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"Routes file not found: {filepath}")

    with open(filepath, "r", encoding="utf-8") as f:
        first_line = f.readline().strip()

    if not first_line.isdigit():
        raise RouteReadError(
            f"Expected integer count on first line of {filepath}, "
            f"got: {first_line!r}"
        )
    return int(first_line)


def read_routes(input_path: str | Path) -> list[RouteRecord]:
    """Read all routes from a file containing a leading count header."""
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"Routes file not found: {filepath}")

    routes: list[RouteRecord] = []

    try:
        with open(filepath, "r", encoding="utf-8") as f:
            count_line = f.readline().strip()
            if not count_line.isdigit():
                raise RouteReadError(
                    f"Expected integer count on first line, got: {count_line!r}"
                )
            expected_count = int(count_line)

            for lineno, line in enumerate(f, start=2):
                line = line.strip()
                if not line:
                    continue

                parts = line.split()
                if len(parts) != 2:
                    raise RouteReadError(
                        f"{filepath}:{lineno}: expected 'prefix port_id', "
                        f"got: {line!r}"
                    )

                prefix, val = parts
                try:
                    mask = int(prefix.split("/")[1])
                except (IndexError, ValueError) as exc:
                    raise RouteReadError(
                        f"{filepath}:{lineno}: cannot parse mask "
                        f"from prefix {prefix!r}"
                    ) from exc

                port_id = int(val) if val.isdigit() else 0
                next_hop = val if not val.isdigit() else ""

                routes.append(
                    RouteRecord(prefix=prefix, mask=mask, port_id=port_id, next_hop=next_hop)
                )

    except OSError as exc:
        raise RouteReadError(
            f"Failed to read routes file '{filepath}': {exc}"
        ) from exc

    if len(routes) != expected_count:
        raise RouteReadError(
            f"{filepath}: count header says {expected_count} records "
            f"but found {len(routes)}"
        )

    return routes


def read_port_map(input_path: str | Path) -> dict[int, str]:
    """Read port map returning {port_id: next_hop}."""
    filepath = Path(input_path)
    if not filepath.is_file():
        raise FileNotFoundError(f"Port map file not found: {filepath}")

    port_map: dict[int, str] = {}

    try:
        with open(filepath, "r", encoding="utf-8") as f:
            count_line = f.readline().strip()
            if not count_line.isdigit():
                raise RouteReadError(
                    f"Expected integer count on first line, got: {count_line!r}"
                )
            expected_count = int(count_line)

            for lineno, line in enumerate(f, start=2):
                line = line.strip()
                if not line:
                    continue

                parts = line.split()
                if len(parts) != 2:
                    raise RouteReadError(
                        f"{filepath}:{lineno}: expected 'port_id next_hop', "
                        f"got: {line!r}"
                    )

                port_str, next_hop = parts
                if not port_str.isdigit():
                    raise RouteReadError(
                        f"{filepath}:{lineno}: port_id must be integer, got: {port_str!r}"
                    )
                port_map[int(port_str)] = next_hop

    except OSError as exc:
        raise RouteReadError(
            f"Failed to read port map '{filepath}': {exc}"
        ) from exc

    if len(port_map) != expected_count:
        raise RouteReadError(
            f"{filepath}: count header says {expected_count} ports "
            f"but found {len(port_map)}"
        )

    return port_map
