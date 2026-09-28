from pathlib import Path

from core.gen_types import RouteRecord
from config import PORT_SIZE_BITS

PORTS_COUNT_MAX = (1 << PORT_SIZE_BITS) - 1

class PortAllocationError(Exception):
    """Raised when available 16-bit ports are exhausted."""
    pass


def map_next_hops_to_ports(
    routes: list[RouteRecord],
    max_ports: int = PORTS_COUNT_MAX,
    start_port: int = 1,
) -> dict[str, int]:
    next_hop_map: dict[str, int] = {}
    current_port = start_port

    for route in routes:
        if route.next_hop not in next_hop_map:
            if current_port > max_ports:
                raise PortAllocationError(
                    f"Exhausted 16-bit port space ({max_ports} ports max). "
                    f"Cannot map next_hop: {route.next_hop}"
                )
            next_hop_map[route.next_hop] = current_port
            current_port += 1

    return next_hop_map


def save_port_map(
    next_hop_map: dict[str, int],
    output_path: str | Path,
) -> None:
    filepath = Path(output_path)
    filepath.parent.mkdir(parents=True, exist_ok=True)

    with open(filepath, "w", encoding="utf-8") as f:
        for next_hop, port_id in next_hop_map.items():
            f.write(f"{next_hop} {port_id}\n")