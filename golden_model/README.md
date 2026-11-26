# Luleå golden model

A cycle-accurate C library for the IPv4 Luleå `18–6–8` lookup. It loads the
nine packed files from `cache_gen` into fixed-size banks using the
[Basys 3 allocation](../stats/cache_stats/TOTALS.md).

## Build

Use the [shared Meson project](../libs_cpp/lulea_verify/README.md) from the repository
root. The C++ lookup test uses the same route fixtures and linear reference
as `cache_gen`.

```sh
meson setup build
meson test -C build popcount pipeline golden_model_lookup --print-errorlogs
```

Include `lulea_model.h` and link `liblulea_model.so` from a C or C++ testbench,
or load the shared library directly from Python. Pass the directory from a
`cache_gen` [dump job](../cache_gen/README.md#quick-start) to `lulea_model_create`.
Loading checks bank capacities, cumulative counts, and pointer bounds.
Each model instance has its own banks and registers.
`cache_images.c` loads and validates the banks. The model in `lulea_model.c`
owns the banks and defines the registers and lookup stages.

## Clocking

Call `lulea_model_tick(model, valid, ip)` once per rising edge and compare
the returned `valid` and `port` against the RTL outputs after that edge.
`ip` is a 32-bit integer with the first octet in bits `[31:24]`.
The pipeline accepts one address per edge. When `valid` is false, that
position in the output stream is a bubble. `lulea_model_reset` clears
the registers and discards in-flight requests. The loaded banks persist.

| Edge for a request | Register | Value captured |
| --- | --- | --- |
| `n` | `l1_rank` | L1 bitmap chunk and chunk sum |
| `n+1` | `l1_seq` | L1 sequence cell |
| `n+2` | `l2_rank` | L2 bitmap chunk and chunk sum, or an L1 port |
| `n+3` | `l2_seq` | L2 sequence cell, or an L1 port |
| `n+4` | `l3_rank` | L3 bitmap chunk and chunk sum, or an earlier port |
| `n+5` | `l3_seq` | Output port and valid bit |

At each level, the sequence address is the chunk sum plus the number of
set bits through the selected bit position, inclusive. The model updates
the registers from L3 back to L1 so each stage reads its predecessor's
pre-edge value. [Datapath](../docs/implementation_18_6_8.drawio)
