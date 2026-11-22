# Selected BRAM allocation

The selected allocation fits every sampled table through 9,216 routes and
2,206 of 3,589 tables (61.47%) at 9,728 routes. [Experiment results](README.md)

| Bank | RAMB18 equivalents | Usable bits | Usable bytes |
|---|---:|---:|---:|
| `l1_pop` | 16 | 262,144 | 32,768 |
| `l1_chunk` | 4 | 73,728 | 9,216 |
| `l1_seq` | 16 | 294,912 | 36,864 |
| `l2_pop` | 32 | 524,288 | 65,536 |
| `l2_chunk` | 8 | 147,456 | 18,432 |
| `l2_seq` | 10 | 184,320 | 23,040 |
| `l3_pop` | 2 | 32,768 | 4,096 |
| `l3_chunk` | 1 | 18,432 | 2,304 |
| `l3_seq` | 1 | 18,432 | 2,304 |
| **Total** | **90** | **1,556,480** | **194,560** |

The allocation reserves **1,658,880 physical bits (207,360 bytes, or
202.5 KiB)** and provides **190 KiB** of usable data. Bitmap banks allocate
two RAMB18 equivalents per 512 words of 64 data bits. The bitmap block count
is rounded up using 32 Kibit of data per RAMB36. Each RAMB36 occupies
36 Kibit of physical memory, including the eight parity bits per word.
