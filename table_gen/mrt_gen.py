import sys
import os
import random
from collections import defaultdict

from .config import BUILD_DIR, ROUTE_COUNT
from .core.gen_types import RouteRecord
from .core import mrt_reader

DEFAULT_INPUT_PATH = "routes.txt"
DEFAULT_OUTPUT_FILE = "mrt_routes.txt"


def parse_and_sample_mrt(
        file_path=DEFAULT_INPUT_PATH,
        output_filename=DEFAULT_OUTPUT_FILE,
        target_count=ROUTE_COUNT,
        file_type=mrt_reader.MRTFileType.TEXT,
        seed=42):

    random.seed(seed)

    print(f"Parsing MRT dump file: {file_path}")
    routes = mrt_reader.read_mrt_routes(file_path, file_type=file_type)

    next_hop_to_id: dict[str, int] = {}
    current_port_id = 1

    for route in routes:
        if route.next_hop not in next_hop_to_id:
            next_hop_to_id[route.next_hop] = current_port_id
            current_port_id += 1

    total_unique = len(routes)
    print(f"Total unique prefixes parsed: {total_unique}")
    print(f"Mapped {len(next_hop_to_id)} unique NEXT_HOPs to port_ids.")

    mask_buckets: dict[int, list[RouteRecord]] = defaultdict(list)
    for route in routes:
        mask_buckets[route.mask].append(route)

    selected_routes: list[RouteRecord] = []
    print("\nMask distribution sampling statistics:")

    for mask in sorted(mask_buckets.keys()):
        bucket = mask_buckets[mask]
        bucket_size = len(bucket)

        quota = round((bucket_size / total_unique) * target_count)

        if quota == 0 and bucket_size > 0:
            quota = 1
        quota = min(quota, bucket_size)

        sampled = random.sample(bucket, quota)
        selected_routes.extend(sampled)

        percentage = (bucket_size / total_unique) * 100
        print(
            f"  /{mask:2d}: Original = {bucket_size:6d} ({percentage:5.2f}%) -> Sampled = {len(sampled):4d}")

    if len(selected_routes) > target_count:
        random.shuffle(selected_routes)
        selected_routes = selected_routes[:target_count]

    print(
        f"\nFinal sampled dataset size: {len(selected_routes)} routes (limit: {target_count})")

    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    output_path = BUILD_DIR / output_filename
    with open(output_path, 'w', encoding='utf-8') as f:
        for route in selected_routes:
            port_id = next_hop_to_id[route.next_hop]
            f.write(f"{route.prefix} {port_id}\n")

    print(f"Base routing table written to '{output_path}'.")


def main():
    file_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_INPUT_PATH

    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)

    parse_and_sample_mrt(file_path)


if __name__ == "__main__":
    main()
