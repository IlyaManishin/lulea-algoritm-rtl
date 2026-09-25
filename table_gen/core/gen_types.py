from dataclasses import dataclass


@dataclass(slots=True)
class RouteRecord:
    prefix: str
    mask: int
    next_hop: str
