# IPv4 Prefix Length Distribution for Luleå Trie Memory Planning

## Motivation

The FPGA implementation of the Luleå routing lookup algorithm is constrained by on-chip memory resources. To allocate memory efficiently across trie levels, it is useful to understand the real-world distribution of IPv4 prefix lengths in the global BGP routing table.

The goal is to optimize memory allocation for the prefix lengths that occur most frequently rather than assuming a uniform distribution.

## Current Internet Routing Statistics

According to RIPE RIS global BGP snapshots from September 2026, the IPv4 routing table contains approximately 1.15 million prefixes.

The most common prefix lengths are:

| Prefix Length | Number of Routes | Share of Routing Table |
| :------------ | ---------------: | --------------------: |
| /24           |          734,899 |                 64.0% |
| /23           |          121,088 |                 10.5% |
| /22           |          118,339 |                 10.3% |
| /21           |           57,238 |                  5.0% |
| /20           |           48,463 |                  4.2% |
| /19           |           26,750 |                  2.3% |
| /18           |           14,046 |                  1.2% |
| /17           |            8,682 |                  0.8% |
| /16           |           13,979 |                  1.2% |

Source: RIPE RIS BGP Full View snapshot (September 2026).  
https://bgp.internet-registry.net/ :contentReference[oaicite:0]{index=0}

## Key Observation

The overwhelming majority of Internet routes are concentrated in a very small range of prefix lengths:

- /24 alone represents approximately **64%** of all IPv4 routes.
- /24, /23, and /22 together represent approximately **85%** of all IPv4 routes.

This observation is consistent with APNIC routing reports, which state that prefixes /24, /23, and /22 account for approximately **83–84%** of the entire IPv4 routing table. :contentReference[oaicite:1]{index=1}

## Implications for Luleå Trie Design

Since the routing table is heavily biased toward longer prefixes:

1. Memory resources should primarily target nodes representing prefixes in the /22–/24 range.
2. Upper trie levels contain relatively few prefixes and therefore require significantly less storage.
3. Compression efficiency is expected to be highest in upper levels because the number of unique prefixes decreases rapidly as prefix length becomes shorter.
4. FPGA memory allocation should prioritize structures associated with deep trie levels, where most routing entries are concentrated.


## References

1. RIPE RIS Global BGP Full View (September 2026)  
   https://bgp.internet-registry.net/ :contentReference[oaicite:2]{index=2}

2. APNIC – BGP in 2024  
   https://blog.apnic.net/2025/01/06/bgp-in-2024/ :contentReference[oaicite:3]{index=3}

3. APNIC – BGP in 2025  
   https://blog.apnic.net/2026/01/08/bgp-in-2025/ :contentReference[oaicite:4]{index=4}