from pathlib import Path
from enum import Enum, auto

BUILD_DIR = Path("build")

ROUTE_COUNT = 4096

class GeneratorType(Enum):
    MRT = auto()
    RANDOM = auto()

