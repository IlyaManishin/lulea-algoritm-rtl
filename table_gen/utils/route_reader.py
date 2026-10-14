"""
Readers for the plain-text route format produced by route_saver.save_routes().

File layout:
    <count>
    <prefix> <next_hop>
    ...
"""

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
    """
    Read all routes from a file produced by save_routes().

    Validates the count header against actual record count for early
    corruption detection.
    """
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
                        f"{filepath}:{lineno}: expected 'prefix next_hop', "
                        f"got: {line!r}"
                    )

                prefix, next_hop = parts
                try:
                    mask = int(prefix.split("/")[1])
                except (IndexError, ValueError) as exc:
                    raise RouteReadError(
                        f"{filepath}:{lineno}: cannot parse mask "
                        f"from prefix {prefix!r}"
                    ) from exc

                routes.append(
                    RouteRecord(prefix=prefix, mask=mask, next_hop=next_hop)
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
