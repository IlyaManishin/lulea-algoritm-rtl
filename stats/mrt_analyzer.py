import sys
import os
import re

DEFAULT_PATH = "routes.txt"

def analyze_mrt_dump(file_path):
    next_hops = set()
    providers_from = set()
    as_paths_origins = set()
    total_records = 0
    unique_prefixes = set()

    current_record = {}

    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.strip()
            
            if not line:
                if current_record:
                    total_records += 1
                    
                    if 'PREFIX' in current_record:
                        unique_prefixes.add(current_record['PREFIX'])
                        
                    if 'NEXT_HOP' in current_record:
                        next_hops.add(current_record['NEXT_HOP'])
                        
                    if 'FROM' in current_record:
                        match = re.search(r'AS(\d+)', current_record['FROM'])
                        if match:
                            providers_from.add(match.group(1))
                            
                    if 'ASPATH' in current_record:
                        as_list = current_record['ASPATH'].split()
                        if as_list:
                            as_paths_origins.add(as_list[0])
                            
                    current_record = {}
                continue

            if ':' in line:
                key, val = line.split(':', 1)
                current_record[key.strip()] = val.strip()

    print("=== Dump Analysis Results ===")
    print(f"Total records in file: {total_records}")
    print(f"Unique prefixes (networks): {len(unique_prefixes)}")
    print(f"Unique NEXT_HOPs (future port_ids): {len(next_hops)}")
    print(f"Unique BGP neighbors (AS from FROM field): {len(providers_from)}")
    print(f"Unique origin ASes (ASPATH origin): {len(as_paths_origins)}")
    
    print("\nFound NEXT_HOPs list (first 10):")
    for nh in list(next_hops)[:10]:
        print(f"  - {nh}")

def main():
    file_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PATH
    
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)

    analyze_mrt_dump(file_path)

if __name__ == "__main__":
    main()