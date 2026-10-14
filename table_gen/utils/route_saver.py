"""Serialisation helpers for route tables and port maps."""

from pathlib import Path

from core.gen_types import RouteRecord


def _ensure_parent(filepath: Path) -> None:
    filepath.parent.mkdir(parents=True, exist_ok=True)


def save_port_map(
    port_map: dict[str, int] | dict[int, str],
    output_path: str | Path,
) -> None:
    """Write port mapping: count + 'port_id next_hop' per line."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(port_map)}\n")
        for k, v in port_map.items():
            if isinstance(k, int):
                f.write(f"{k} {v}\n")
            else:
                f.write(f"{v} {k}\n")


def save_route_table(
    routes: list[RouteRecord],
    port_map: dict[str, int] | None,
    output_path: str | Path,
) -> None:
    """Write compiled table: count + 'prefix port_id' per line."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(routes)}\n")
        for route in routes:
            pid = route.port_id if route.port_id else (port_map[route.next_hop] if port_map else 0)
            f.write(f"{route.prefix} {pid}\n")


def save_routes_and_ports(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    routes_output_path: str | Path,
    port_map_output_path: str | Path,
) -> None:
    """Save route table (prefix port_id) + port map (port_id next_hop)."""
    save_port_map(port_map, port_map_output_path)
    save_route_table(routes, port_map, routes_output_path)


def save_routes(
    routes: list[RouteRecord],
    output_path: str | Path,
) -> None:
    """Write routes: count + 'prefix port_id' per line."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(routes)}\n")
        for route in routes:
            f.write(f"{route.prefix} {route.port_id}\n")


def save_converted(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    routes_path: str | Path,
    port_map_path: str | Path,
) -> None:
    """Save converted routes (prefix port_id) + port map (port_id next_hop)."""
    for route in routes:
        if not route.port_id and route.next_hop in port_map:
            route.port_id = port_map[route.next_hop]
    save_routes(routes, routes_path)
    save_port_map(port_map, port_map_path)
