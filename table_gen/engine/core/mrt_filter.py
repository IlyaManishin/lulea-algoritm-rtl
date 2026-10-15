"""Stratified sampling filter for MRT route tables based on prefix length distribution."""

from collections import defaultdict
import random

from .gen_types import MrtRecord, RouteRecord


def filter_mrt_routes[T: (RouteRecord, MrtRecord)](
    routes: list[T],
    target_count: int,
    seed: int | None = None,
) -> list[T]:
    """
    Sample routes while preserving mask length distribution (Hamilton method).
    """
    if not routes or target_count <= 0:
        return []
    if target_count >= len(routes):
        return list(routes)

    rng = random.Random(seed)

    # 1. Group routes by mask length into buckets
    buckets: dict[int, list[T]] = defaultdict(list)
    for route in routes:
        buckets[route.mask].append(route)

    # 2. Calculate fair share for each mask (integer quota + fractional remainder)
    total_routes = len(routes)
    quotas: dict[int, int] = {}
    remainders: dict[int, float] = {}

    for mask, bucket in buckets.items():
        share = (len(bucket) / total_routes) * target_count
        quotas[mask] = int(share)
        remainders[mask] = share - quotas[mask]

    # 3. Distribute missing routes to masks with the largest remainder
    missing_routes = target_count - sum(quotas.values())
    sorted_masks = sorted(
        buckets.keys(),
        key=lambda m: (remainders[m], rng.random()),
        reverse=True,
    )

    for mask in sorted_masks:
        if missing_routes <= 0:
            break
        if quotas[mask] < len(buckets[mask]):
            quotas[mask] += 1
            missing_routes -= 1

    # 4. Sample routes from each bucket according to its quota
    selected_routes: list[T] = []
    for mask, quota in quotas.items():
        if quota > 0:
            selected_routes.extend(rng.sample(buckets[mask], quota))

    rng.shuffle(selected_routes)
    return selected_routes
