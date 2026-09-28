from pathlib import Path
from enum import Enum, auto

BUILD_DIR = Path("build")
DEFAULT_MRT_PATH = ""


ROUTE_COUNT = 4096
PORT_SIZE_BITS = 8

PORT_MAP_EXTENSION = ".ports"


class GeneratorType(Enum):
    MRT = auto()
    RANDOM = auto()

