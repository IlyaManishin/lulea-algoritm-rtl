from pathlib import Path

from core.gen_types import RouteRecord


def save_port_map(
    port_map: dict[str, int],
    output_path: str | Path,
) -> None:
    filepath = Path(output_path)
    filepath.parent.mkdir(parents=True, exist_ok=True)

    with open(filepath, "w", encoding="utf-8") as f:
        for next_hop, port_id in port_map.items():
            f.write(f"{next_hop} {port_id}\n")


def save_route_table(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    output_path: str | Path,
) -> None:
    filepath = Path(output_path)
    filepath.parent.mkdir(parents=True, exist_ok=True)

    with open(filepath, "w", encoding="utf-8") as f:
        for route in routes:
            f.write(f"{route.prefix} {port_map[route.next_hop]}\n")


def save_routes_and_ports(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    routes_output_path: str | Path,
    port_map_output_path: str | Path,
) -> None:
    save_port_map(port_map, port_map_output_path)
    save_route_table(routes, port_map, routes_output_path)
