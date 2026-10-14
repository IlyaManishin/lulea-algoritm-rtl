from dataclasses import dataclass


@dataclass(slots=True)
class MrtRecord:
    prefix: str
    mask: int
    next_hop: str


@dataclass(slots=True)
class RouteRecord:
    prefix: str
    mask: int
    port_id: int
