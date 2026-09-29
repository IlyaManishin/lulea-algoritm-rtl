import os
from pathlib import Path
from dotenv import load_dotenv

ENV_PATH = Path(__file__).resolve().parent.parent / ".env"

load_dotenv(dotenv_path=ENV_PATH)

# Path to the directory containing MRT files
MRT_SOURCE_DIR = os.getenv("MRT_SOURCE_DIR")


def get_mrt_paths() -> list[Path]:
    """Return a list of paths for all items in MRT_SOURCE_DIR without recursion or filtering."""
    if not MRT_SOURCE_DIR:
        raise ValueError("MRT_SOURCE_DIR is not set in environment or .env file")

    source_path = Path(MRT_SOURCE_DIR)

    if not source_path.exists() or not source_path.is_dir():
        raise FileNotFoundError(
            f"Directory {source_path} does not exist or is not a directory"
        )

    return list(source_path.iterdir())