from pathlib import Path

from .gen_types import MrtRecord
from .mrt_filter import filter_mrt_routes
from .mrt_reader import read_mrt_routes, MRTFileType

# Backwards compatibility alias
distribution_filter_mrt_routes = filter_mrt_routes


def generate_mrt_routes(
    input_path: str | Path,
    count: int,
    file_type: MRTFileType = MRTFileType.TEXT,
    seed: int | None = 42,
    output_path: str | Path | None = None,
) -> list[MrtRecord]:
    routes: list[MrtRecord] = read_mrt_routes(input_path, file_type=file_type)
    if not routes:
        return []

    sampled_routes: list[MrtRecord] = filter_mrt_routes(
        routes, target_count=count, seed=seed
    )

    if output_path is not None:
        filepath = Path(output_path)
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, "w", encoding="utf-8") as f:
            for route in sampled_routes:
                f.write(f"{route.prefix} {route.next_hop}\n")

    return sampled_routes