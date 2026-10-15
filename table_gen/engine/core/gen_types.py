import socket
import struct
from dataclasses import dataclass

type PortMapping = dict[int, str]


@dataclass(frozen=True, slots=True)
class MrtRecord:
    prefix: str
    mask: int
    next_hop: str

    def __post_init__(self) -> None:
        if not (0 <= self.mask <= 32):
            raise ValueError(f"Invalid mask: {self.mask} (must be 0..32)")
        if not self.prefix or "/" not in self.prefix:
            raise ValueError(f"Invalid prefix: {self.prefix}")


@dataclass(frozen=True, slots=True)
class RouteRecord:
    prefix: str
    mask: int
    port_id: int

    def __post_init__(self) -> None:
        if not (0 <= self.mask <= 32):
            raise ValueError(f"Invalid mask: {self.mask} (must be 0..32)")
        if self.port_id < 0 or self.port_id > 255:
            raise ValueError(f"Invalid port_id: {self.port_id} (must be 0..255)")

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
