"""Stratified sampling filter for MRT route tables based on prefix length distribution."""

from collections import defaultdict
import random

from .gen_types import MrtRecord, RouteRecord


def filter_mrt_routes(
    routes: list[RouteRecord | MrtRecord],
    target_count: int,
    seed: int | None = None,
) -> list[RouteRecord | MrtRecord]:
    """Sample routes while preserving mask length distribution."""
    if seed is not None:
        random.seed(seed)

    total_unique = len(routes)
    if total_unique == 0:
        return []

    mask_buckets: dict[int, list[RouteRecord | MrtRecord]] = defaultdict(list)
    for route in routes:
        mask_buckets[route.mask].append(route)

    selected_routes: list[RouteRecord | MrtRecord] = []

    for mask in sorted(mask_buckets.keys()):
        bucket = mask_buckets[mask]
        bucket_size = len(bucket)

        # NOTE: Forcing quota=1 guarantees rare buckets are included.
        # However, for small target_count, this overrepresents rare items
        # and distorts the sample compared to the original distribution.
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
