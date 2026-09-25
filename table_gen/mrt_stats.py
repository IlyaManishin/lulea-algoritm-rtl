import sys
from pathlib import Path
from collections import Counter
from core.mrt_reader import read_mrt_routes, MRTFileType

DEFAULT_PATH = ""


def analyze_mrt_routes(file_path: str | Path) -> None:
    routes = read_mrt_routes(file_path, file_type=MRTFileType.BINARY)

    total_records = len(routes)
    next_hops = set()
    mask_distribution = Counter()

    for route in routes:
        next_hops.add(route.next_hop)
        mask_distribution[route.mask] += 1

    print("=== Binary MRT Analysis Results ===")
    print(f"Total unique routes parsed: {total_records}")
    print(f"Unique NEXT_HOPs: {len(next_hops)}")

    print("\nFound NEXT_HOPs list (first 10):")
    for nh in list(next_hops)[:10]:
        print(f"  - {nh}")

    print("\n=== Mask Length Distribution ===")
    total_valid_prefixes = sum(mask_distribution.values())

    if total_valid_prefixes > 0:
        print(f"{'Mask':<8} | {'Count':<10} | {'Percentage':<10}")
        print("-" * 34)
        for mask_len in sorted(mask_distribution.keys()):
            count = mask_distribution[mask_len]
            percentage = (count / total_valid_prefixes) * 100
            print(f"/{mask_len:<7} | {count:<10} | {percentage:>8.2f}%")
        print("-" * 34)
        print(f"{'Total':<8} | {total_valid_prefixes:<10} | 100.00%")
    else:
        print("No valid prefix masks found.")


def main():
    file_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PATH

    path = Path(file_path)
    if not path.exists():
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)

    analyze_mrt_routes(path)


if __name__ == "__main__":
    main()