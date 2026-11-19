# Architectural Decision Record: Luleå Algorithm Chunk Size Selection (32-bit vs. 64-bit)

## 1. Problem Statement
When implementing the Luleå routing lookup algorithm on a resource-constrained FPGA (Xilinx Artix-7 XC7A35T on Basys 3, limited to 50 RAMB36 blocks / 225 KB total BRAM), selecting the bitmap chunk size ($W \in \{16, 32, 64\}$ bits) introduces a fundamental trade-off between **BRAM consumption** and **combinational popcount delay**:

* **Larger Chunks (64-bit):** Halve the number of entries required in the prefix-sum array (`chunk_sums`) compared to 32-bit chunks, significantly reducing total BRAM consumption. However, a 64-bit popcount tree increases combinational delay on the critical path.
* **Smaller Chunks (32-bit / 16-bit):** Reduce the popcount logic delay and yield higher standalone $F_{\max}$, but double (or quadruple) the depth of the `chunk_sums` array, quickly exhausting on-chip BRAM at higher routing table scales.

---

## 2. Timing Benchmark Summary (Artix-7 `-1`)
Synthesis and implementation benchmarks across 12 configurations (evaluating 16-bit, 32-bit, and 64-bit widths, with and without pipeline isolation at $100\text{ MHz}$ and $200\text{ MHz}$ constraints) yielded the following timing metrics:

| BRAM Width | Pipeline | Target Clock | $T_{\text{req}}$ (ns) | WNS (ns) | $F_{\max}$ (MHz) | Critical Path (ns) | Stage 1: BRAM $\rightarrow$ Pipe (ns) | Stage 2: Pipe $\rightarrow$ Out (ns) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **16-bit** | `nopipe` | $200\text{ MHz}$ | $5.0$ | $+0.267$ | $211.28$ | $4.621$ | — | — |
| **16-bit** | `pipe` | $200\text{ MHz}$ | $5.0$ | $+1.358$ | $274.57$ | $3.541$ | $2.647$ | $3.541$ |
| **32-bit** | `nopipe` | $200\text{ MHz}$ | $5.0$ | $+0.317$ | $213.54$ | $4.614$ | — | — |
| **32-bit** | `pipe` | $200\text{ MHz}$ | $5.0$ | $+0.626$ | $228.62$ | $4.303$ | $2.006$ | $4.303$ |
| **64-bit** | `nopipe` | $100\text{ MHz}$ | $10.0$ | $+0.937$ | $110.34$ | $8.925$ | — | — |
| **64-bit** | `nopipe` | $200\text{ MHz}$ | $5.0$ | $-0.667$ | $176.46$ | $5.520$ | — | — |
| **64-bit** | `pipe` | $100\text{ MHz}$ | $10.0$ | $+2.749$ | $137.91$ | $7.134$ | $2.389$ | $7.134$ |
| **64-bit** | `pipe` | $200\text{ MHz}$ | $5.0$ | $-0.439$ | $183.86$ | $5.356$ | $2.332$ | $5.356$ |

---

## 3. Final Decision: 64-bit Chunk Configuration with Pipelining

We select the **64-bit chunk size (`USE_BRAM_64`)** with pipeline registers enabled (`IS_PIPELINED`) as the baseline architecture.

### Key Rationale
1. **BRAM Capacity is a Hard Physical Ceiling; Timing is Soft:**
   On the Artix-7 XC7A35T, on-chip BRAM is the primary bottleneck limiting the maximum supported routing table size (`limit`). Choosing a 64-bit chunk size cuts the `chunk_sums` array length in half compared to 32-bit chunks, saving substantial BRAM resources on Levels 2 and 3. Conversely, a longer combinational critical path can always be mitigated architecturally via pipelining.
2. **Manageable Timing Gap ($5.356\text{ ns}$ vs. $4.303\text{ ns}$):**
   Under aggressive synthesis constraints ($200\text{ MHz}$), the pipelined 64-bit configuration achieves $F_{\max} = 183.86\text{ MHz}$ (critical path of $5.356\text{ ns}$), which is only $\approx 1.05\text{ ns}$ slower than the 32-bit pipelined version ($4.303\text{ ns}$, $228.62\text{ MHz}$) and comfortably exceeds our system target of $125\text{–}150\text{ MHz}$.
3. **Clear Path to Further Timing Optimization:**
   The stage breakdown for the 64-bit pipelined run shows that **Stage 1 (`BRAM -> douta_pipe`) takes only $2.332\text{ ns}$**, while **Stage 2 (`douta_pipe -> popcount_out`) takes $5.356\text{ ns}$** (of which $3.319\text{ ns}$ is routing delay and $0.744\text{ ns}$ is logic delay). If higher clock frequencies ($>185\text{ MHz}$) are required later in full-system integration, Stage 2 can be trivially split with a single mid-tree pipeline register inside the 64-bit popcount adder at virtually zero cost.