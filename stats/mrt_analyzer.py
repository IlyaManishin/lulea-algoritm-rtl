from collections import Counter
import os
import re
import sys

DEFAULT_PATH = "routes.txt"


def parse_mrt_record(record: dict, stats: dict) -> None:
    if "PREFIX" not in record:
        return

    prefix = record["PREFIX"]

    if prefix in stats["unique_prefixes"]:
        return

    stats["unique_prefixes"].add(prefix)
    stats["total_records"] += 1

    if "/" in prefix:
        try:
            mask_len = int(prefix.split("/")[1])
            stats["mask_distribution"][mask_len] += 1
        except ValueError:
            pass

    if "NEXT_HOP" in record:
        stats["next_hops"].add(record["NEXT_HOP"])

    if "FROM" in record:
        match = re.search(r"AS(\d+)", record["FROM"])
        if match:
            stats["providers_from"].add(match.group(1))

    if "ASPATH" in record:
        as_list = record["ASPATH"].split()
        if as_list:
            stats["as_paths_origins"].add(as_list[0])


def read_mrt_dump(file_path: str) -> dict:
    stats = {
        "total_records": 0,
        "unique_prefixes": set(),
        "next_hops": set(),
        "providers_from": set(),
        "as_paths_origins": set(),
        "mask_distribution": Counter(),
    }

    current_record = {}

    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()

            if not line:
                if current_record:
                    parse_mrt_record(current_record, stats)
                    current_record = {}
                continue

            if ":" in line:
                key, val = line.split(":", 1)
                current_record[key.strip()] = val.strip()

        if current_record:
            parse_mrt_record(current_record, stats)

    return stats


def print_analysis_results(stats: dict) -> None:
    print("=== Dump Analysis Results (Unique Prefixes Only) ===")
    print(f"Total unique records: {stats['total_records']}")
    print(f"Unique prefixes (networks): {len(stats['unique_prefixes'])}")
    print(f"Unique NEXT_HOPs (future port_ids): {len(stats['next_hops'])}")
    print(
        f"Unique BGP neighbors (AS from FROM field): {len(stats['providers_from'])}"
    )
    print(
        f"Unique origin ASes (ASPATH origin): {len(stats['as_paths_origins'])}"
    )

    print("\nFound NEXT_HOPs list (first 10):")
    for nh in list(stats["next_hops"])[:10]:
        print(f"  - {nh}")

    print("\n=== Mask Length Distribution ===")
    total_valid_prefixes = sum(stats["mask_distribution"].values())

    if total_valid_prefixes > 0:
        print(f"{'Mask':<8} | {'Count':<10} | {'Percentage':<10}")
        print("-" * 34)
        for mask_len in sorted(stats["mask_distribution"].keys()):
            count = stats["mask_distribution"][mask_len]
            percentage = (count / total_valid_prefixes) * 100
            print(f"/{mask_len:<7} | {count:<10} | {percentage:>8.2f}%")
        print("-" * 34)
        print(f"{'Total':<8} | {total_valid_prefixes:<10} | 100.00%")
    else:
        print("No valid prefix masks found.")


def main():
    file_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PATH

    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)

    stats = read_mrt_dump(file_path)
    print_analysis_results(stats)


if __name__ == "__main__":
    main()