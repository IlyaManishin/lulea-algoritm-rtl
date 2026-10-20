from dataclasses import dataclass
import math

DEFAULT_PORT_SIZE = 8


@dataclass
class LevelConfig:
    bits: int
    chunk_size: int

    @property
    def cells_count(self) -> int:
        return 1 << self.bits

    @property
    def chunks_count(self) -> int:
        return self.cells_count // self.chunk_size

    def __post_init__(self):
        if self.chunk_size <= 0 or (self.chunk_size & (self.chunk_size - 1)) != 0:
            raise ValueError(
                f"chunk_size must be a power of 2, got {self.chunk_size}"
            )


@dataclass
class LuleaConfig:
    name: str
    l1: LevelConfig
    l2: LevelConfig
    l3: LevelConfig
    dist: tuple[float, float, float]
    port_size_bits: int = DEFAULT_PORT_SIZE

    def __post_init__(self):
        if not math.isclose(sum(self.dist), 1.0):
            raise ValueError(
                f"Sum of proportions {self.dist} must equal 1.0"
            )


LULEA_CONFIGS = [
    LuleaConfig(
        name="16-8-8",
        l1=LevelConfig(bits=16, chunk_size=64),
        l2=LevelConfig(bits=8, chunk_size=16),
        l3=LevelConfig(bits=8, chunk_size=16),
        dist=(0.005, 0.990, 0.005),
    ),
    LuleaConfig(
        name="18-6-8",
        l1=LevelConfig(bits=18, chunk_size=64),
        l2=LevelConfig(bits=6, chunk_size=16),
        l3=LevelConfig(bits=8, chunk_size=16),
        dist=(0.033, 0.962, 0.005),
    ),
    LuleaConfig(
        name="20-4-8",
        l1=LevelConfig(bits=20, chunk_size=64),
        l2=LevelConfig(bits=4, chunk_size=16),
        l3=LevelConfig(bits=8, chunk_size=16),
        dist=(0.100, 0.895, 0.005),
    ),
]