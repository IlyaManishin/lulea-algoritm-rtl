from dataclasses import dataclass, field
import math


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
    overhead_kb: float
    seq_kb: float
    total_kb: float


# =========================================================================
# Core Memory Calculation
# =========================================================================

def calculate_memory_for_limit(
    limit: int, cfg: LuleaConfig
) -> SimulationResult:
    # 1. Distribute masks across levels
    p1, p2, p3 = cfg.dist
    masks_l1 = limit * p1
    masks_l2 = limit * p2
    masks_l3 = limit * p3

    # 2. Estimate node counts
    l1_nodes = 1
    l2_nodes = max(1, int(masks_l2))
    l3_nodes = max(1, int(masks_l3))

    # 3. Calculate reference bit-widths
    ref_l2_bits = math.ceil(math.log2(l2_nodes)) if l2_nodes > 1 else 1
    ref_l3_bits = math.ceil(math.log2(l3_nodes)) if l3_nodes > 1 else 1

    # 4. Calculate cell sizes
    cell1_bits = 1 + max(cfg.port_size_bits, ref_l2_bits)
    cell2_bits = 1 + max(cfg.port_size_bits, ref_l3_bits)
    cell3_bits = 1 + cfg.port_size_bits

    # 5. Calculate overhead (bitmaps + chunk sums)
    overhead1_bits = l1_nodes * (
        cfg.l1.total_entries + cfg.l1.chunks_count * 16
    )
    overhead2_bits = l2_nodes * (
        cfg.l2.total_entries + cfg.l2.chunks_count * 16
    )
    overhead3_bits = l3_nodes * (
        cfg.l3.total_entries + cfg.l3.chunks_count * 16
    )

    # 6. Estimate maptable lengths and sizes
    seq1_len = min(
        l1_nodes * cfg.l1.total_entries, int(masks_l1 * 2 + l2_nodes)
    )
    seq2_len = min(
        l2_nodes * cfg.l2.total_entries, int(masks_l2 * 2 + l3_nodes)
    )
    seq3_len = min(l3_nodes * cfg.l3.total_entries, int(masks_l3 * 2))

    seq1_bits = seq1_len * cell1_bits
    seq2_bits = seq2_len * cell2_bits
    seq3_bits = seq3_len * cell3_bits

    # 7. Convert bits to KB
    overhead_kb = (overhead1_bits + overhead2_bits + overhead3_bits) / 8192
    seq_kb = (seq1_bits + seq2_bits + seq3_bits) / 8192
    total_kb = overhead_kb + seq_kb

    return SimulationResult(
        limit=limit,
        l2_nodes=l2_nodes,
        l3_nodes=l3_nodes,
        cell1_bits=cell1_bits,
        cell2_bits=cell2_bits,
        cell3_bits=cell3_bits,
        overhead_kb=overhead_kb,
        seq_kb=seq_kb,
        total_kb=total_kb,
    )


# =========================================================================
# Simulation Runner & Output
# =========================================================================

def run_simulation(cfg: LuleaConfig, limits: list[int]):
    print(f"=== Configuration: {cfg.name} ===")
    print(
        f"{'Limit':>8} | {'L2 Nodes':>8} | {'L3 Nodes':>8} | {'C1':>3} | {'C2':>3} | {'Overhead(KB)':>12} | {'Seq(KB)':>12} | {'Total(KB)':>12}"
    )
    print("-" * 84)

    for limit in limits:
        res = calculate_memory_for_limit(limit, cfg)
        print(
            f"{res.limit:>8} | {res.l2_nodes:>8} | {res.l3_nodes:>8} | "
            f"{res.cell1_bits:>3} | {res.cell2_bits:>3} | "
            f"{res.overhead_kb:>12.2f} | {res.seq_kb:>12.2f} | {res.total_kb:>12.2f}"
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
