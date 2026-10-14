"""
Serialisation helpers for route tables and port maps.

All output files start with an integer count header on the first line,
followed by one record per line.

Two route formats are supported:

  Compiled  (save_route_table):  "prefix port_id"   — used by golden model / RTL.
  Raw       (save_routes):       "prefix next_hop"   — intermediate pipeline format.
"""

from pathlib import Path

from core.gen_types import RouteRecord


def _ensure_parent(filepath: Path) -> None:
    filepath.parent.mkdir(parents=True, exist_ok=True)


# =========================================================================
# Port map
# =========================================================================

def save_port_map(
    port_map: dict[str, int],
    output_path: str | Path,
) -> None:
    """Write port mapping: count + 'next_hop port_id' per line."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(port_map)}\n")
        for next_hop, port_id in port_map.items():
            f.write(f"{next_hop} {port_id}\n")


# =========================================================================
# Compiled route table (prefix → numeric port)
# =========================================================================

def save_route_table(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    output_path: str | Path,
) -> None:
    """Write compiled table: count + 'prefix port_id' per line."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(routes)}\n")
        for route in routes:
            f.write(f"{route.prefix} {port_map[route.next_hop]}\n")


def save_routes_and_ports(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    routes_output_path: str | Path,
    port_map_output_path: str | Path,
) -> None:
    """Save compiled route table + port map side by side."""
    save_port_map(port_map, port_map_output_path)
    save_route_table(routes, port_map, routes_output_path)


# =========================================================================
# Raw routes (prefix → text next_hop, for batch pipeline intermediate files)
# =========================================================================

def save_routes(
    routes: list[RouteRecord],
    output_path: str | Path,
) -> None:
    """Write intermediate routes: count + 'prefix next_hop' per line."""
    filepath = Path(output_path)
    _ensure_parent(filepath)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"{len(routes)}\n")
        for route in routes:
            f.write(f"{route.prefix} {route.next_hop}\n")


def save_converted(
    routes: list[RouteRecord],
    port_map: dict[str, int],
    routes_path: str | Path,
    port_map_path: str | Path,
) -> None:
    """Save raw routes + port map (stage 1 → stage 2 handoff)."""
    save_routes(routes, routes_path)
    save_port_map(port_map, port_map_path)
