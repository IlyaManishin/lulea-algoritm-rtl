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

ENV_PATH = PRJ_ROOT_DIR / ".env"
load_dotenv(dotenv_path=ENV_PATH)


# Path to the directory containing MRT files
MRT_SOURCE_DIR = os.getenv("MRT_SOURCE_DIR")


class GeneratorType(Enum):
    MRT = auto()
    RANDOM = auto()
