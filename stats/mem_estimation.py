"""
Luleå Algorithm BRAM Memory Consumption Simulator.

Estimates bitmap and chunk sum sizes, cell sizes (pointer vs port ID widths),
and total memory usage for 16-8-8, 18-6-8, and 20-4-8 Luleå routing table
configurations across varying route limit scales (2^10 to 2^21).
"""

from dataclasses import dataclass
import math


# =========================================================================
# Configuration Constants
# =========================================================================

MAX_NODE_FILL_FACTOR = 0.5  # part of endpoint in certain cache limit
BRAM_BASE_CELL_SIZE = 18

# Flag to enable/disable rounding node counts to power of 2
ROUND_NODES_TO_POWER_OF_2 = True

# =========================================================================
# Data Structures & Models
# =========================================================================


@dataclass
class LevelConfig:
    bits: int
    chunk_size: int

    @property
    def total_entries(self) -> int:
        return 1 << self.bits

    @property
    def chunks_count(self) -> int:
        return self.total_entries // self.chunk_size

    def __post_init__(self):
        if self.chunk_size <= 0 or (self.chunk_size & (self.chunk_size - 1)) != 0:
            raise ValueError(
                f"chunk_size must be a power of 2, got {self.chunk_size}"
            )


@dataclass
class LuleaConfig:
    name: str
    l1: LevelConfig
    l2: LevelConfig
    l3: LevelConfig
    dist: tuple[float, float, float]
    port_size_bits: int = 16

    def __post_init__(self):
        if not math.isclose(sum(self.dist), 1.0):
            raise ValueError(
                f"Sum of proportions {self.dist} must equal 1.0"
            )


@dataclass
class SimulationResult:
    limit: int
    l2_nodes: int
    l3_nodes: int
    cell1_bits: int
    cell2_bits: int
    cell3_bits: int
    pop_arr_kb: float
    chunk_sums_bits: float
    seq_kb: float
    total_kb: float


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
    """ Estimate real entries count in view of limits
    """
    masks_l2 = limit * cfg.dist[1]
    masks_l3 = limit * cfg.dist[2]

    max_l2_nodes = int(cfg.l1.total_entries * fill_factor)
    max_l3_nodes = int(max_l2_nodes * cfg.l2.total_entries * fill_factor)

    l1_nodes = 1
    l2_nodes = min(max_l2_nodes, int(masks_l2))
    l3_nodes = min(max_l3_nodes, int(masks_l3))

    if ROUND_NODES_TO_POWER_OF_2:
        l2_nodes = round_up_pow2(l2_nodes)
        l3_nodes = round_up_pow2(l3_nodes)

    return l1_nodes, l2_nodes, l3_nodes


def estimate_ref_bits(
    cfg: LuleaConfig, l2_nodes: int, l3_nodes: int
) -> tuple[int, int, int]:
    """ Count ref cell size (flag + port_id or next cache level pointer)
    """
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


def estimate_chunk_cell_bits(cfg: LuleaConfig) -> tuple[int, int, int]:
    """ Count chunk cell size
    """
    chunk_cell1_bits = align_by_bram_cell_size(cfg.l1.chunk_size)
    chunk_cell2_bits = align_by_bram_cell_size(cfg.l2.chunk_size)
    chunk_cell3_bits = align_by_bram_cell_size(cfg.l3.chunk_size)

    return chunk_cell1_bits, chunk_cell2_bits, chunk_cell3_bits


def estimate_seq_lens(
    cfg: LuleaConfig, l1_nodes: int, l2_nodes: int, l3_nodes: int
) -> tuple[int, int, int]:
    """ Estimate ref array lengths
    """
    seq1_len = min(
        l1_nodes * cfg.l1.total_entries, int(l1_nodes * 2 + l2_nodes)
    )
    seq2_len = min(
        l2_nodes * cfg.l2.total_entries, int(l2_nodes * 2 + l3_nodes)
    )
    seq3_len = min(
        l3_nodes * cfg.l3.total_entries, int(l3_nodes * 2)
    )

    return seq1_len, seq2_len, seq3_len


def estimate_seq_ptr_cell_bits(
    seq1_len: int, seq2_len: int, seq3_len: int
) -> tuple[int, int, int]:
    """ Count seq pointer cell size
    """
    ptr1_raw = math.ceil(math.log2(max(1, seq1_len)))
    ptr2_raw = math.ceil(math.log2(max(1, seq2_len)))
    ptr3_raw = math.ceil(math.log2(max(1, seq3_len)))

    seq_ptr1_bits = align_by_bram_cell_size(max(1, ptr1_raw))
    seq_ptr2_bits = align_by_bram_cell_size(max(1, ptr2_raw))
    seq_ptr3_bits = align_by_bram_cell_size(max(1, ptr3_raw))

    return seq_ptr1_bits, seq_ptr2_bits, seq_ptr3_bits


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
    chunk_cell1_bits, chunk_cell2_bits, chunk_cell3_bits = estimate_chunk_cell_bits(
        cfg
    )

    # -------------------------------------------------------------------------
    # 2. Reference Sequence Lengths & Pointer Sizes
    # -------------------------------------------------------------------------
    seq1_len, seq2_len, seq3_len = estimate_seq_lens(
        cfg, l1_nodes, l2_nodes, l3_nodes
    )

    seq_ptr1_bits, seq_ptr2_bits, seq_ptr3_bits = estimate_seq_ptr_cell_bits(
        seq1_len, seq2_len, seq3_len
    )

    # -------------------------------------------------------------------------
    # 3. Popcount Arrays Size
    # -------------------------------------------------------------------------
    pop_arr1_bits = l1_nodes * cfg.l1.chunks_count * chunk_cell1_bits
    pop_arr2_bits = l2_nodes * cfg.l2.chunks_count * chunk_cell2_bits
    pop_arr3_bits = l3_nodes * cfg.l3.chunks_count * chunk_cell3_bits

    pop_arr_bits = pop_arr1_bits + pop_arr2_bits + pop_arr3_bits

    # -------------------------------------------------------------------------
    # 4. Chunk Sums Size
    # -------------------------------------------------------------------------
    chunk_sums1_bits = l1_nodes * cfg.l1.chunks_count * seq_ptr1_bits
    chunk_sums2_bits = l2_nodes * cfg.l2.chunks_count * seq_ptr2_bits
    chunk_sums3_bits = l3_nodes * cfg.l3.chunks_count * seq_ptr3_bits

    chunk_sums_bits = chunk_sums1_bits + chunk_sums2_bits + chunk_sums3_bits

    # -------------------------------------------------------------------------
    # 5. Reference Sequences Size
    # -------------------------------------------------------------------------
    seq1_bits = seq1_len * ref1_bits
    seq2_bits = seq2_len * ref2_bits
    seq3_bits = seq3_len * ref3_bits

    seq_bits = seq1_bits + seq2_bits + seq3_bits

    # -------------------------------------------------------------------------
    # 6. Total Aggregation (Bits to KB)
    # -------------------------------------------------------------------------
    pop_arr_kb = pop_arr_bits / 8192
    chunk_sums_kb = chunk_sums_bits / 8192
    seq_kb = seq_bits / 8192
    total_kb = pop_arr_kb + chunk_sums_kb + seq_kb

    return SimulationResult(
        limit=limit,
        l2_nodes=l2_nodes,
        l3_nodes=l3_nodes,
        cell1_bits=ref1_bits,
        cell2_bits=ref2_bits,
        cell3_bits=ref3_bits,
        pop_arr_kb=pop_arr_kb,
        chunk_sums_bits=chunk_sums_kb,
        seq_kb=seq_kb,
        total_kb=total_kb,
    )

# =========================================================================
# Simulation Runner & Output
# =========================================================================


def run_simulation(cfg: LuleaConfig, limits: list[int]):
    print(f"=== Configuration: {cfg.name} ===")
    print(
        f"{'Limit':>8} | {'L2 Nodes':>8} | {'L3 Nodes':>8} | {'C1':>3} | {'C2':>3} | "
        f"{'Pop_arr(KB)':>12} | {'Ch_sums(KB)':>12} | {'Seq(KB)':>12} | {'Total(KB)':>12}"
    )
    print("-" * 99)

    for limit in limits:
        res = calculate_memory_for_limit(limit, cfg)
        print(
            f"{res.limit:>8} | {res.l2_nodes:>8} | {res.l3_nodes:>8} | "
            f"{res.cell1_bits:>3} | {res.cell2_bits:>3} | "
            f"{res.pop_arr_kb:>12.2f} | {res.chunk_sums_bits:>12.2f} | "
            f"{res.seq_kb:>12.2f} | {res.total_kb:>12.2f}"
        )
    print("\n")


# =========================================================================
# Main Entrypoint
# =========================================================================

def main():
    configs = [
        LuleaConfig(
            name="16-8-8",
            l1=LevelConfig(bits=16, chunk_size=64),
            l2=LevelConfig(bits=8, chunk_size=16),
            l3=LevelConfig(bits=8, chunk_size=16),
            dist=(0.005, 0.990, 0.005),
        ),
        LuleaConfig(
            name="18-6-8",
            l1=LevelConfig(bits=18, chunk_size=64),
            l2=LevelConfig(bits=6, chunk_size=16),
            l3=LevelConfig(bits=8, chunk_size=16),
            dist=(0.033, 0.962, 0.005),
        ),
        LuleaConfig(
            name="20-4-8",
            l1=LevelConfig(bits=20, chunk_size=64),
            l2=LevelConfig(bits=4, chunk_size=16),
            l3=LevelConfig(bits=8, chunk_size=16),
            dist=(0.100, 0.895, 0.005),
        ),
    ]

    limits = [2**i for i in range(10, 22)]

    for cfg in configs:
        run_simulation(cfg, limits)


if __name__ == "__main__":
    main()
