from pathlib import Path
from enum import Enum, auto
import os
from dotenv import load_dotenv


TABLE_GEN_DIR = Path(__file__).resolve().parent
PRJ_ROOT_DIR = TABLE_GEN_DIR.parent

BUILD_DIR = PRJ_ROOT_DIR / "build"
DEFAULT_MRT_PATH = ""

ROUTE_COUNT = 4096
PORT_SIZE_BITS = 8

PORT_MAP_EXTENSION = ".ports"

# Batch pipeline paths
CONVERTED_DIR = BUILD_DIR / "converted"
SAMPLED_DIR = BUILD_DIR / "sampled"

# Batch pipeline settings
TABLE_SIZES: list[int] = [1024, 2048, 4096, 8192, 16384]

# Fraction of the source table to consume across all samples for a given size.
SAMPLE_FILL_COEFF: float = 1.0

ROUTE_FILENAME = "routes.txt"
PORT_MAP_FILENAME = "routes.ports"
INFO_FILENAME = "info.txt"

ENV_PATH = PRJ_ROOT_DIR / ".env"
load_dotenv(dotenv_path=ENV_PATH)

MRT_SOURCE_DIR = os.getenv("MRT_SOURCE_DIR")


class GeneratorType(Enum):
    MRT = auto()
    RANDOM = auto()
