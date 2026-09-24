import sys
import os
import re
import random
from collections import defaultdict
from pathlib import Path

# Import configuration parameters
from config import BUILD_DIR, ROUTE_COUNT

DEFAULT_INPUT_PATH = "routes.txt"
OUTPUT_FILE = BUILD_DIR / "lulea_base_table.txt"

def parse_and_sample_mrt(file_path, target_count=ROUTE_COUNT, seed=42):
    random.seed(seed)
    
    unique_routes = {}
    next_hop_to_id = {}
    current_port_id = 1

    print(f"Parsing MRT dump file: {file_path}")
    current_record = {}

    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.strip()
            
            if not line:
                if current_record:
                    prefix = current_record.get('PREFIX')
                    next_hop = current_record.get('NEXT_HOP')
                    
                    if prefix and next_hop and prefix not in unique_routes:
                        unique_routes[prefix] = next_hop
                        if next_hop not in next_hop_to_id:
                            next_hop_to_id[next_hop] = current_port_id
                            current_port_id += 1
                            
                    current_record = {}
                continue

            if ':' in line:
                key, val = line.split(':', 1)
                current_record[key.strip()] = val.strip()

    total_unique = len(unique_routes)
    print(f"Total unique prefixes parsed: {total_unique}")
    print(f"Mapped {len(next_hop_to_id)} unique NEXT_HOPs to port_ids.")

    # Group routes by exact mask size (1 to 32)
    mask_buckets = defaultdict(list)
    for prefix, nh_ip in unique_routes.items():
        mask = int(prefix.split('/')[1])
        mask_buckets[mask].append((prefix, nh_ip))

    # Stratified sampling: calculate quota for each exact mask length
    selected_routes = []
    print("\nMask distribution sampling statistics:")
    
    for mask in sorted(mask_buckets.keys()):
        bucket = mask_buckets[mask]
        bucket_size = len(bucket)
        
        # Calculate exact quota for this mask size
        quota = round((bucket_size / total_unique) * target_count)
        
        # Handle edge cases (at least 1 item if quota rounds to 0, but don't exceed bucket size)
        if quota == 0 and bucket_size > 0:
            quota = 1
        quota = min(quota, bucket_size)

        sampled = random.sample(bucket, quota)
        selected_routes.extend(sampled)
        
        percentage = (bucket_size / total_unique) * 100
        print(f"  /{mask:2d}: Original = {bucket_size:6d} ({percentage:5.2f}%) -> Sampled = {len(sampled):4d}")

    # Adjust sample size if rounding produced slightly more/fewer than target_count
    if len(selected_routes) > target_count:
        random.shuffle(selected_routes)
        selected_routes = selected_routes[:target_count]

    print(f"\nFinal sampled dataset size: {len(selected_routes)} routes (limit: {target_count})")

    # Ensure output directory exists
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    # Export base routing table: PREFIX/MASK PORT_ID
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        for prefix, nh_ip in selected_routes:
            port_id = next_hop_to_id[nh_ip]
            f.write(f"{prefix} {port_id}\n")

    print(f"Base routing table written to '{OUTPUT_FILE}'.")

def main():
    file_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_INPUT_PATH
    
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)

    parse_and_sample_mrt(file_path)

if __name__ == "__main__":
    main()