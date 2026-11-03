"""
Luleå Algorithm BRAM Memory Consumption Simulator.

Estimates bitmap and chunk sum sizes, cell sizes (pointer vs port ID widths),
and total memory usage for Luleå routing table configurations across varying
route limit scales (2^10 to 2^21).
"""

import math

from lulea_configurations import LULEA_CONFIGS, LuleaConfig
from mem_stats_saver import (
    ConfigResult,
    LevelMemory,
    SimulationResult,
    save_config_results_to_csv,
    save_configs_totals_csv,
    save_limit_comparison_csvs,
)

# =========================================================================
# Configuration Constants
# =========================================================================

MAX_NODE_FILL_FACTOR = 0.5  # part of endpoint in certain cache limit
BRAM_BASE_CELL_SIZE = 9

# Flag to enable/disable rounding node counts to power of 2
ROUND_NODES_TO_POWER_OF_2 = True


# =========================================================================
# Core Memory Calculation
# =========================================================================

def align_by_bram_cell_size(raw_size: int) -> int:
    res = BRAM_BASE_CELL_SIZE
    while res < raw_size:
        res *= 2
    return res


def round_up_pow2(n: int) -> int:
    """Round up integer to the next power of 2."""
    if n <= 0:
        return 0
    return 1 << (n - 1).bit_length()


def estimate_node_counts(
    limit: int, cfg: LuleaConfig, fill_factor: float = MAX_NODE_FILL_FACTOR
) -> tuple[int, int, int]:
    """Estimate real entries count in view of limits."""
    l1_nodes = 1

    masks_l2 = limit * cfg.dist[1]
    masks_l3 = limit * cfg.dist[2]

    l1_slots = cfg.l1.cells_count
    l2_slots_per_node = cfg.l2.cells_count

    # NOTE: At small masks_l2/masks_l3 count (when these is much less than max masks capacity):
    # expected_l3_nodes ≈ masks_l3. It needs to check real cases in route tables.

    # Expected number of non-empty L1 slots (= L2 nodes actually needed)
    # when masks_l2 prefixes are thrown uniformly at random into l1_slots bins.
    expected_l2_nodes = l1_slots * (1 - (1 - 1 / l1_slots) ** masks_l2)

    max_l2_nodes = int(l1_slots * fill_factor)
    l2_nodes = min(max_l2_nodes, int(expected_l2_nodes))

    # Same logic one level down: L3 nodes needed out of the address space
    # opened up by however many L2 nodes actually got allocated.
    l3_size = max(1, l2_nodes * l2_slots_per_node)
    expected_l3_nodes = l3_size * (1 - (1 - 1 / l3_size) ** masks_l3)

    max_l3_nodes = int(l2_nodes * cfg.l2.cells_count * fill_factor)
    l3_nodes = min(max_l3_nodes, int(expected_l3_nodes))

    if ROUND_NODES_TO_POWER_OF_2:
        l2_nodes = round_up_pow2(l2_nodes)
        l3_nodes = round_up_pow2(l3_nodes)

    return l1_nodes, l2_nodes, l3_nodes


def estimate_ref_bits(
    cfg: LuleaConfig, l2_nodes: int, l3_nodes: int
) -> tuple[int, int, int]:
    """Count ref cell size (flag + port_id or next cache level pointer)."""
    ptr_l2_bits = math.ceil(math.log2(l2_nodes)) if l2_nodes > 1 else 1
    ptr_l3_bits = math.ceil(math.log2(l3_nodes)) if l3_nodes > 1 else 1

    cell1_bits_raw = 1 + max(cfg.port_size_bits, ptr_l2_bits)
    cell2_bits_raw = 1 + max(cfg.port_size_bits, ptr_l3_bits)
    cell3_bits_raw = 1 + cfg.port_size_bits

    # Align by BRAM cell size
    cell1_bits = align_by_bram_cell_size(cell1_bits_raw)
    cell2_bits = align_by_bram_cell_size(cell2_bits_raw)
    cell3_bits = align_by_bram_cell_size(cell3_bits_raw)

    return cell1_bits, cell2_bits, cell3_bits


def estimate_chunk_size_aligned(cfg: LuleaConfig) -> tuple[int, int, int]:
    """Count aligned chunk size."""
    chunk_size1_aligned = align_by_bram_cell_size(cfg.l1.chunk_size)
    chunk_size2_aligned = align_by_bram_cell_size(cfg.l2.chunk_size)
    chunk_size3_aligned = align_by_bram_cell_size(cfg.l3.chunk_size)

    return chunk_size1_aligned, chunk_size2_aligned, chunk_size3_aligned


def estimate_seq_lens(
    cfg: LuleaConfig, l1_nodes: int, l2_nodes: int, l3_nodes: int
) -> tuple[int, int, int]:
    """Estimate ref array lengths."""
    seq1_len = min(
        l1_nodes * cfg.l1.cells_count, int(l1_nodes * 2 + l2_nodes)
    )
    seq2_len = min(
        l2_nodes * cfg.l2.cells_count, int(l2_nodes * 2 + l3_nodes)
    )
    seq3_len = min(
        l3_nodes * cfg.l3.cells_count, int(l3_nodes * 2)
    )

    return seq1_len, seq2_len, seq3_len


def estimate_seq_ptr_size(
    seq1_len: int, seq2_len: int, seq3_len: int
) -> tuple[int, int, int]:
    """Count seq pointer size."""
    ptr1_raw = math.ceil(math.log2(max(1, seq1_len)))
    ptr2_raw = math.ceil(math.log2(max(1, seq2_len)))
    ptr3_raw = math.ceil(math.log2(max(1, seq3_len)))

    seq_ptr1_size = align_by_bram_cell_size(max(1, ptr1_raw))
    seq_ptr2_size = align_by_bram_cell_size(max(1, ptr2_raw))
    seq_ptr3_size = align_by_bram_cell_size(max(1, ptr3_raw))

    return seq_ptr1_size, seq_ptr2_size, seq_ptr3_size


def calculate_memory_for_limit(
    limit: int, cfg: LuleaConfig
) -> SimulationResult:
    # -------------------------------------------------------------------------
    # 1. Base Node Counts & Cell Sizes
    # -------------------------------------------------------------------------
    l1_nodes, l2_nodes, l3_nodes = estimate_node_counts(limit, cfg)

    ref1_bits, ref2_bits, ref3_bits = estimate_ref_bits(
        cfg, l2_nodes, l3_nodes
    )
    chunk_size1_aligned, chunk_size2_aligned, chunk_size3_aligned = (
        estimate_chunk_size_aligned(cfg)
    )

    # -------------------------------------------------------------------------
    # 2. Reference Sequence Lengths & Pointer Sizes
    # -------------------------------------------------------------------------
    seq1_len, seq2_len, seq3_len = estimate_seq_lens(
        cfg, l1_nodes, l2_nodes, l3_nodes
    )

    seq_ptr1_size, seq_ptr2_size, seq_ptr3_size = estimate_seq_ptr_size(
        seq1_len, seq2_len, seq3_len
    )

    # -------------------------------------------------------------------------
    # 3. Popcount Arrays Size
    # -------------------------------------------------------------------------
    pop_arr1_bits = l1_nodes * cfg.l1.chunks_count * chunk_size1_aligned
    pop_arr2_bits = l2_nodes * cfg.l2.chunks_count * chunk_size2_aligned
    pop_arr3_bits = l3_nodes * cfg.l3.chunks_count * chunk_size3_aligned

    pop_arr_bits = pop_arr1_bits + pop_arr2_bits + pop_arr3_bits

    # -------------------------------------------------------------------------
    # 4. Chunk Sums Size
    # -------------------------------------------------------------------------
    chunk_sums1_bits = l1_nodes * cfg.l1.chunks_count * seq_ptr1_size
    chunk_sums2_bits = l2_nodes * cfg.l2.chunks_count * seq_ptr2_size
    chunk_sums3_bits = l3_nodes * cfg.l3.chunks_count * seq_ptr3_size

    chunk_sums_bits = chunk_sums1_bits + chunk_sums2_bits + chunk_sums3_bits

    # -------------------------------------------------------------------------
    # 5. Reference Sequences Size
    # -------------------------------------------------------------------------
    seq1_bits = seq1_len * ref1_bits
    seq2_bits = seq2_len * ref2_bits
    seq3_bits = seq3_len * ref3_bits

    seq_bits = seq1_bits + seq2_bits + seq3_bits

    # -------------------------------------------------------------------------
    # 6. Memory Stats by Levels
    # -------------------------------------------------------------------------
    l1_memory = LevelMemory(
        pop_arr_kb=round(pop_arr1_bits / 8192, 1),
        chunk_sums_kb=round(chunk_sums1_bits / 8192, 1),
        seq_kb=round(seq1_bits / 8192, 1),
        seq_len=seq1_len,
    )
    l2_memory = LevelMemory(
        pop_arr_kb=round(pop_arr2_bits / 8192, 1),
        chunk_sums_kb=round(chunk_sums2_bits / 8192, 1),
        seq_kb=round(seq2_bits / 8192, 1),
        seq_len=seq2_len,
    )
    l3_memory = LevelMemory(
        pop_arr_kb=round(pop_arr3_bits / 8192, 1),
        chunk_sums_kb=round(chunk_sums3_bits / 8192, 1),
        seq_kb=round(seq3_bits / 8192, 1),
        seq_len=seq3_len,
    )

    # -------------------------------------------------------------------------
    # 7. Total Aggregation (Bits to KB)
    # -------------------------------------------------------------------------
    pop_arr_kb = round(pop_arr_bits / 8192, 1)
    chunk_sums_kb = round(chunk_sums_bits / 8192, 1)
    seq_kb = round(seq_bits / 8192, 1)
    total_kb = round(pop_arr_kb + chunk_sums_kb + seq_kb, 1)

    return SimulationResult(
        limit=limit,
        l2_nodes=l2_nodes,
        l3_nodes=l3_nodes,
        cell1_bits=ref1_bits,
        cell2_bits=ref2_bits,
        cell3_bits=ref3_bits,
        pop_arr_kb=pop_arr_kb,
        chunk_sums_kb=chunk_sums_kb,
        seq_kb=seq_kb,
        total_kb=total_kb,
        l1=l1_memory,
        l2=l2_memory,
        l3=l3_memory,
    )


# =========================================================================
# Simulation Runner & Output
# =========================================================================

def run_simulation(cfg: LuleaConfig, limits: list[int]) -> ConfigResult:
    print(f"=== Configuration: {cfg.name} ===")
    print(
        f"{'Limit':>8} | {'L2 Nodes':>8} | {'L3 Nodes':>8} | {'C1':>3} | {'C2':>3} | "
        f"{'Pop_arr(KB)':>12} | {'Ch_sums(KB)':>12} | {'Seq(KB)':>12} | {'Total(KB)':>12}"
    )
    print("-" * 99)

    results = []
    for limit in limits:
        res = calculate_memory_for_limit(limit, cfg)
        results.append(res)

        print(
            f"{res.limit:>8} | {res.l2_nodes:>8} | {res.l3_nodes:>8} | "
            f"{res.cell1_bits:>3} | {res.cell2_bits:>3} | "
            f"{res.pop_arr_kb:>12.1f} | {res.chunk_sums_kb:>12.1f} | "
            f"{res.seq_kb:>12.1f} | {res.total_kb:>12.1f}"
        )
    print("\n")
    return ConfigResult(config_name=cfg.name, results=results)


# =========================================================================
# Main Entrypoint
# =========================================================================

def main():
    limits = [2**i for i in range(10, 22)]
    all_config_results = []

    for cfg in LULEA_CONFIGS:
        cfg_result = run_simulation(cfg, limits)
        save_config_results_to_csv(cfg_result)
        all_config_results.append(cfg_result)

    save_limit_comparison_csvs(all_config_results, limits)
    save_configs_totals_csv(all_config_results, limits)


if __name__ == "__main__":
    main()
