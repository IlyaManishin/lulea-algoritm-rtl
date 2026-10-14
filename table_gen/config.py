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

# -------------------------------------------------------------------------
# Batch pipeline directories
# -------------------------------------------------------------------------

# Stage 1: fully-parsed MRT tables (all unique routes, no sampling)
CONVERTED_DIR = BUILD_DIR / "converted"

# Stage 2: sampled sub-tables produced from converted tables
SAMPLED_DIR = BUILD_DIR / "sampled"

# -------------------------------------------------------------------------
# Batch pipeline parameters
# -------------------------------------------------------------------------

# Target sizes for sampled tables (route counts)
TABLE_SIZES: list[int] = [1024, 2048, 4096, 8192, 16384]

# Fraction of the source table to consume across all samples for a given size.
# coeff = (target_size * n_samples) / total_routes
# coeff=1.0 means we consume exactly as many routes in total as the table has.
# coeff=2.0 means routes are reused ~2x across the samples.
SAMPLE_FILL_COEFF: float = 1.0

# -------------------------------------------------------------------------
# Shared file names inside each converted/sampled directory
# -------------------------------------------------------------------------

ROUTE_FILENAME = "routes.txt"
PORT_MAP_FILENAME = "routes.ports"

ENV_PATH = PRJ_ROOT_DIR / ".env"
load_dotenv(dotenv_path=ENV_PATH)


# Path to the directory containing MRT files
MRT_SOURCE_DIR = os.getenv("MRT_SOURCE_DIR")


class GeneratorType(Enum):
    MRT = auto()
    RANDOM = auto()
