from collections import defaultdict
from pathlib import Path
import random

from .gen_types import RouteRecord
from .mrt_reader import read_mrt_routes, MRTFileType


def sample_routes_by_mask(
    routes: list[RouteRecord],
    target_count: int,
    seed: int | None = None,
) -> list[RouteRecord]:
    if seed is not None:
        random.seed(seed)

    total_unique = len(routes)
    if total_unique == 0:
        return []

    mask_buckets: dict[int, list[RouteRecord]] = defaultdict(list)
    for route in routes:
        mask_buckets[route.mask].append(route)

    selected_routes: list[RouteRecord] = []

    for mask in sorted(mask_buckets.keys()):
        bucket = mask_buckets[mask]
        bucket_size = len(bucket)

        quota = round((bucket_size / total_unique) * target_count)
        if quota == 0 and bucket_size > 0:
            quota = 1
        quota = min(quota, bucket_size)

        sampled = random.sample(bucket, quota)
        selected_routes.extend(sampled)

    if len(selected_routes) > target_count:
        random.shuffle(selected_routes)
        selected_routes = selected_routes[:target_count]

    return selected_routes


def generate_mrt_routes(
    input_path: str | Path,
    count: int,
    file_type: MRTFileType = MRTFileType.TEXT,
    seed: int | None = 42,
    output_path: str | Path | None = None,
) -> list[RouteRecord]:
    routes = read_mrt_routes(input_path, file_type=file_type)
    if not routes:
        return []

    sampled_routes = sample_routes_by_mask(
        routes, target_count=count, seed=seed
    )

    if output_path is not None:
        filepath = Path(output_path)
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, "w", encoding="utf-8") as f:
            for route in sampled_routes:
                f.write(f"{route.prefix} {route.next_hop}\n")

    return sampled_routes