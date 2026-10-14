import socket
import struct
from dataclasses import dataclass

type PortMapping = dict[int, str]


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

    @property
    def ip_str(self) -> str:
        return self.prefix.split("/")[0]

    @property
    def ip_int(self) -> int:
        return struct.unpack(">I", socket.inet_aton(self.ip_str))[0]

    @classmethod
    def from_raw(cls, ip_int: int, mask: int, port_id: int) -> "RouteRecord":
        ip_str = socket.inet_ntoa(struct.pack(">I", ip_int))
        return cls(prefix=f"{ip_str}/{mask}", mask=mask, port_id=port_id)
