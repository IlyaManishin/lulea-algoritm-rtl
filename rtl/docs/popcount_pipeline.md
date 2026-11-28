# Luleå popcount pipeline

The selected implementation is **`mask-register`** (`3978426ba`). It adds one
stage to each of L1, L2, and L3. It closes setup and hold timing at a 6.670 ns
clock period and gives the most even split of the measured paths inside L1.
The full pipeline accepts one IPv4 address per cycle. A result now appears on
the twelfth rising edge, counting the edge that accepts the request as the first.

## Original three-stage level

| Edge | Result captured |
| --- | --- |
| `n` | POP and CHUNK ROM outputs, `ip_pop`, and `valid_pop`. |
| `n+1` | `offset_left_r` and `offset_right_r` from the two 32-bit popcounts of `pop_word & mask`, along with `chunk_sum_r`. |
| `n+2` | SEQ ROM output. `seq_addr` is `chunk_sum_r + offset_left_r + offset_right_r`. |

L1 already divides the work between two substantial paths. From `u_l1_pop`
to `offset_left_r` the data path takes **7.351 ns** (−0.709 ns setup slack).
From `offset_left_r` to `u_l1_seq/ENARDEN` it takes **6.549 ns** (−0.440 ns).
The path to `u_l1_seq/ADDRARDADDR[13]` takes 5.966 ns (−0.156 ns). The first two
numbers add to 13.900 ns, a rough measure of the work in those two stages.
They are worst paths in separate cycles and include clock-to-output delay,
logic, and routing. Across the design, the worst path starts at `u_l2_pop`
and ends at `offset_right_r` at −1.120 ns setup slack.

The first rising edge launches the ROM read. A fourth stage helps when it
removes work from the interval between that edge and the capture of the
partial popcounts. Registering another value after `offset_left_r` and
`offset_right_r` alone would leave the ROM-to-popcount path intact. The useful
cuts tested here either defer counting until after a register or compute
smaller popcounts before that register and reduce them in the next cycle.

## Selected four-stage level

| Edge | Result captured |
| --- | --- |
| `n` | POP and CHUNK ROM outputs, `ip_pop`, and `valid_pop`. |
| `n+1` | `masked_word_r <= pop_word & mask`, with `chunk_sum_s1`, `ip_s1`, and `valid_s1`. |
| `n+2` | The two 32-bit popcounts in `offset_left_r` and `offset_right_r`, with `chunk_sum_s2`, `ip_s2`, and `valid_s2`. |
| `n+3` | SEQ ROM output at `seq_addr`. |

`mask` includes bits zero through `bit_pos`. The added register captures the
masked bitmap so the POP ROM output no longer has to pass through mask logic
and the 32-bit popcount in the same cycle. The bypass paths in `pipeline.sv`
carry `valid`, IP, and port for the extra cycle per skipped level so early
and late matches keep the same end-to-end latency and request order.

## Timing across the experiments

All runs used Vivado 2025.2, the `xc7a35tcpg236-1` target, the same nine ROM
images from `rib.20251101.0000` (RouteViews, 9,216 routes, `seed_58`), and a
**6.670 ns** clock constraint. The baseline is `7cdeb5198`. Results are
after `opt_design`, `place_design`, and
`route_design`. The timing comparison covers clocked internal paths. The
board constraints provide no external input or output delay budget. All
variants used 41.5 BRAM tiles in these runs.

### Worst setup path in each design

`u_lN_pop`, `u_lN_chunk`, and `u_lN_seq` are instances under `u_cache`.
The `*_reg` cells for mask and rank are under `u_pipeline/u_lN/u_seq_addr`.
Delay is Vivado's data path delay for the reported worst setup path. WNS
and WHS are the worst setup and hold slack for the design.

| Variant | Change from the baseline | Worst setup path | Delay (ns) | WNS (ns) | WHS (ns) |
| --- | --- | --- | ---: | ---: | ---: |
| Baseline | Two 32-bit popcounts after the full bitmap mask | `u_l2_pop/dout_reg__0/CLKARDCLK` → L2 `offset_right_r_reg[4]/D` | 7.756 | −1.120 | +0.101 |
| `local-mask` | Decode the mask in 8-bit groups and keep the two 32-bit popcounts in the same stage | `u_l2_pop/dout_reg__4/CLKARDCLK` → L2 `offset_right_r_reg[3]/D` | 7.655 | −1.103 | +0.066 |
| `early-mask` | Register the mask on the POP/CHUNK read edge using the incoming `bit_pos` | `u_l2_pop/dout_reg__2/CLKARDCLK` → L2 `offset_right_r_reg[4]/D` | 7.495 | −0.974 | +0.027 |
| **`mask-register`** | **Register `masked_word` before the two 32-bit popcounts** | `u_l1_seq/dout_reg__5/CLKARDCLK` → `u_l2_pop/dout_reg__8/ENARDEN` | 6.039 | **+0.034** | **+0.023** |
| `4bit` (mask first) | Mask each 4-bit group, count it, and register the result before reduction | `u_l1_seq/dout_reg__4/CLKARDCLK` → `u_l2_pop/dout_reg__5/ENARDEN` | 5.964 | −0.094 | +0.138 |
| `8bit` | Register eight masked 8-bit popcounts, then reduce four per half | L2 `chunk_sum_r_reg[2]/C` → `u_l2_seq/dout_reg__2/ENARDEN` | 5.819 | +0.044 | +0.067 |
| `16bit` | Register four masked 16-bit popcounts, then reduce two per half | `u_l1_seq/dout_reg__4/CLKARDCLK` → `u_l2_pop/dout_reg__7/ENARDEN` | 5.885 | +0.077 | +0.123 |
| `rom-register` | Add output registers to POP and CHUNK ROMs before masking and counting | `u_l1_seq/g_direct_output.dout_reg__2/CLKARDCLK` → `u_l2_pop/g_registered_output.rom_word_reg__13/ADDRARDADDR[8]` | 6.281 | −0.362 | +0.108 |
| `8bit` (registered full rank) | Reduce the eight registered group counts to one registered `offset_r`, then add `chunk_sum_r` | `u_l1_seq/dout_reg__5/CLKARDCLK` → `u_l2_pop/dout_reg__6/ENARDEN` | 6.031 | −0.133 | +0.084 |
| `4bit` (parallel counts) | Compute the full and locally masked popcounts of each 4-bit group in parallel. Register the full count for earlier groups, the local count for the selected group, and zero for later groups. | `u_l1_seq/dout_reg__5/CLKARDCLK` → `u_l2_pop/dout_reg/ENARDEN` | 5.882 | +0.184 | +0.044 |

The baseline, `local-mask`, and `early-mask` retain three stages per level
and nine cycles end to end. All other variants use four stages per level
and twelve cycles. Verilator checked the output IP, port, and valid bit
against a longest-prefix-match reference over 60,005 cycles per run,
including continuous requests, bubbles, and reset with requests in flight.

### Balance inside L1

For each stage, the table shows the data path delay of the path with the
worst setup slack in that part of the design. The setup slack is in
parentheses. The first register holds `masked_word_r` in `mask-register`
and `group_rank_r` in the grouped variants. The timing reports use the
original synthesized register names.

| Variant | `u_l1_pop` → first register (ns) | First register → `offset_*_r` (ns) | `offset_*_r` / `chunk_sum_r` → `u_l1_seq` (ns) |
| --- | ---: | ---: | ---: |
| **`mask-register`** | **5.797 (+0.722)** | **5.660 (+1.015)** | **5.763 (+0.444)** |
| `8bit` | 5.816 (+0.694) | 5.313 (+1.269) | 5.369 (+0.631) |
| `16bit` | 6.426 (+0.151) | 3.722 (+2.769) | 5.514 (+0.408) |
| `4bit` (parallel counts) | 4.900 (+1.679) | 5.802 (+0.746) | 5.564 (+0.362) |

The `mask-register` split stays within **0.137 ns** across the three L1
paths. The ROM output and mask dominate the first stage. The second stage
has four LUT levels on its worst path, with 4.390 ns of routing. The third
stage adds the half-counts and chunk sum and drives the SEQ ROM. This is a
measured balance of roughly 5.7–5.8 ns per stage.

The 8-bit split puts more logic before the first register, but leaves
only four counts per half to reduce. The 16-bit split pushes more work into
the first stage and leaves a single addition per half in the second stage.
In the parallel 4-bit variant, the local count has four bitmap bits and
two position bits as inputs, which is why the experiment targeted LUT6s.
The worst L1 path from `u_l1_pop` to `group_rank_r` uses two LUT levels.
The next stage must reduce eight counts per half, with routing accounting
for 4.500 ns of that 5.802 ns path.

## Decision

Keep `mask-register` for the current 150 MHz target. It has the most even
L1 partition, reuses the existing two 32-bit popcounts, and uses 627 slice
LUTs versus 686 in the baseline and 826 in the parallel 4-bit variant. Its
worst path now crosses from `u_l1_seq` to `u_l2_pop/ENARDEN`. Of the 6.039 ns
on that path, 3.337 ns is routing. The +0.034 ns setup and +0.023 ns hold
slack are small and apply to this routed design and ROM image.

The parallel 4-bit variant (`rtl-popcount-4bit`, `5ab40358b`) has the best overall setup slack
of the experiments. Rechecking its existing routed checkpoint at 6.500 ns
gave +0.014 ns setup slack. At 6.450 ns it gave −0.036 ns. The difference in
maximum frequency comes from the inter-level path in these runs. The
selected masked-word register gives the more even partition inside the level.
