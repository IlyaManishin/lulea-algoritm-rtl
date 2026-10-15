from ..core.gen_types import MrtRecord, PortMapping
from ..config import PORTS_COUNT_MAX


class PortAllocationError(Exception):
    """Raised when available ports are exhausted."""
    pass


def map_next_hops_to_ports(
    routes: list[MrtRecord],
    max_ports: int = PORTS_COUNT_MAX,
    start_port: int = 1,
) -> PortMapping:
    """Map unique next_hop IPs to sequential port IDs, returning {port_id: next_hop}."""
    hop_to_port: dict[str, int] = {}
    port_map: PortMapping = {}
    current_port = start_port

    for route in routes:
        if route.next_hop not in hop_to_port:
            if current_port > max_ports:
                raise PortAllocationError(
                    f"Exhausted port space ({max_ports} ports max). "
                    f"Cannot map next_hop: {route.next_hop}"
                )
            hop_to_port[route.next_hop] = current_port
            port_map[current_port] = route.next_hop
            current_port += 1

    return port_map

