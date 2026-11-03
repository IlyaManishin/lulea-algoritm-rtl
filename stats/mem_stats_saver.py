from dataclasses import asdict, dataclass
from pathlib import Path
import pandas as pd

from config import RESULTS_DIR

# =========================================================================
# Output Configuration Constants
# =========================================================================

OUTPUT_DIR = Path(RESULTS_DIR) / "mem_estimation"
LIMITS_OUTPUT_DIR = OUTPUT_DIR / "limits_out"

SHORTED_OUT_COLUMNS = [
    "limit",
    "l2_nodes",
    "l3_nodes",
    "cell1_bits",
    "cell2_bits",
    "pop_arr_kb",
    "chunk_sums_kb",
    "seq_kb",
    "total_kb",
]

# =========================================================================
# Data Structures & Models
# =========================================================================


@dataclass
class LevelMemory:
    pop_arr_kb: float
    chunk_sums_kb: float
    seq_kb: float
    seq_len: int

    @property
    def total_kb(self) -> float:
        return round(self.pop_arr_kb + self.chunk_sums_kb + self.seq_kb, 1)


@dataclass
class SimulationResult:
    limit: int
    l2_nodes: int
    l3_nodes: int
    cell1_bits: int
    cell2_bits: int
    cell3_bits: int
    pop_arr_kb: float
    chunk_sums_kb: float
    seq_kb: float
    total_kb: float
    l1: LevelMemory
    l2: LevelMemory
    l3: LevelMemory

    @property
    def seq1_len(self) -> int:
        return self.l1.seq_len

    @property
    def seq2_len(self) -> int:
        return self.l2.seq_len

    @property
    def seq3_len(self) -> int:
        return self.l3.seq_len


@dataclass
class ConfigResult:
    config_name: str
    results: list[SimulationResult]


# =========================================================================
# CSV Persistence Functions
# =========================================================================

def save_config_results_to_csv(config_result: ConfigResult) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Full version
    filepath = OUTPUT_DIR / f"{config_result.config_name}.csv"
    df = pd.json_normalize([asdict(r) for r in config_result.results], sep="_")
    df.to_csv(filepath, index=False)

    # Short version
    short_filepath = OUTPUT_DIR / f"{config_result.config_name}_short.csv"
    df[SHORTED_OUT_COLUMNS].to_csv(short_filepath, index=False)


def save_limit_comparison_csvs(
    config_results: list[ConfigResult], limits: list[int]
) -> None:
    LIMITS_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    for limit in limits:
        limit_rows = []
        for cfg_res in config_results:
            sim_res = next(r for r in cfg_res.results if r.limit == limit)
            row_dict = {"config_name": cfg_res.config_name}
            row_dict.update(asdict(sim_res))
            limit_rows.append(row_dict)

        df = pd.json_normalize(limit_rows, sep="_")
        filepath = LIMITS_OUTPUT_DIR / f"limit_{limit}.csv"
        df.to_csv(filepath, index=False)

        # Short version
        short_cols = ["config_name"] + SHORTED_OUT_COLUMNS
        short_filepath = LIMITS_OUTPUT_DIR / f"limit_{limit}_short.csv"
        df[short_cols].to_csv(short_filepath, index=False)


def save_configs_totals_csv(
    config_results: list[ConfigResult], limits: list[int]
) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    totals_data = {"limit": limits}
    for cfg_res in config_results:
        totals_data[cfg_res.config_name] = [r.total_kb for r in cfg_res.results]

    df_totals = pd.DataFrame(totals_data)
    filepath = OUTPUT_DIR / "configs_totals.csv"
    df_totals.to_csv(filepath, index=False)
