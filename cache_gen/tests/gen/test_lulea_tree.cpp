#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "gen/config.hpp"
#include "gen/core/cache.hpp"
#include "gen/core/lulea_tree.hpp"
#include "gen/engine/lulea_tree_lookup.hpp"
#include "gen/io/reader.hpp"
#include "gen/lulea_config.hpp"

namespace
{
    gen::port_t tree_lookup(const gen::LuleaTree &tree, gen::ipv4_t ip)
    {
        return engine::LuleaTreeLookup(tree).lookup(ip);
    }

    struct BlockPair
    {
        size_t lhs_idx = 0;
        size_t rhs_idx = 0;
    };

    gen::port_t reference_lpm(const gen::MaskEntries &entries, gen::ipv4_t ip)
    {
        int best_mask = -1;
        gen::port_t best_port = gen::NULL_PORT;

        for (const auto &entry : entries)
        {
            const size_t mask_len = std::to_integer<size_t>(entry.mask);
            bool matches = false;
            if (mask_len == 0)
            {
                matches = true;
            }
            else if (mask_len <= 32)
            {
                const uint32_t net_mask = ~uint32_t{0} << (32 - mask_len);
                matches = (ip & net_mask) == (entry.ip & net_mask);
            }

            if (matches && static_cast<int>(mask_len) >= best_mask)
            {
                best_mask = static_cast<int>(mask_len);
                best_port = entry.port;
            }
        }

        return best_port;
    }

    gen::TMaskEntry make_entry(gen::ipv4_t ip, uint8_t mask, uint8_t port)
    {
        return gen::TMaskEntry{
            .ip = ip,
            .mask = static_cast<gen::mask_t>(mask),
            .port = static_cast<gen::port_t>(port),
        };
    }

    bool configs_equal(const gen::LuleaTreeConfig &lhs, const gen::LuleaTreeConfig &rhs)
    {
        return lhs.l1_bits == rhs.l1_bits &&
               lhs.l2_bits == rhs.l2_bits &&
               lhs.l3_bits == rhs.l3_bits &&
               lhs.l1_cell_bits == rhs.l1_cell_bits &&
               lhs.l2_cell_bits == rhs.l2_cell_bits &&
               lhs.l3_cell_bits == rhs.l3_cell_bits;
    }

    bool l3_blocks_equal(const gen::LuleaRoute &lhs_block, const gen::LuleaRoute &rhs_block)
    {
        if (lhs_block.size() != rhs_block.size() ||
            lhs_block.cell_size_bits() != rhs_block.cell_size_bits())
        {
            return false;
        }
        for (size_t idx = 0; idx < lhs_block.size(); ++idx)
        {
            const gen::LuleaCell &lhs_cell = lhs_block[idx];
            const gen::LuleaCell &rhs_cell = rhs_block[idx];
            if (lhs_cell.is_ptr() || rhs_cell.is_ptr() || lhs_cell.port() != rhs_cell.port())
            {
                return false;
            }
        }
        return true;
    }

    bool l2_blocks_equal(
        const gen::LuleaTree &lhs,
        const gen::LuleaTree &rhs,
        BlockPair pair)
    {
        if (pair.lhs_idx >= lhs.l2().size() || pair.rhs_idx >= rhs.l2().size())
        {
            return false;
        }
        const gen::LuleaRoute &lhs_block = lhs.l2()[pair.lhs_idx];
        const gen::LuleaRoute &rhs_block = rhs.l2()[pair.rhs_idx];
        if (lhs_block.size() != rhs_block.size() ||
            lhs_block.cell_size_bits() != rhs_block.cell_size_bits())
        {
            return false;
        }
        for (size_t idx = 0; idx < lhs_block.size(); ++idx)
        {
            const gen::LuleaCell &lhs_cell = lhs_block[idx];
            const gen::LuleaCell &rhs_cell = rhs_block[idx];
            if (lhs_cell.is_ptr() != rhs_cell.is_ptr())
            {
                return false;
            }
            if (lhs_cell.is_ptr())
            {
                if (lhs_cell.ptr() >= lhs.l3().size() || rhs_cell.ptr() >= rhs.l3().size())
                {
                    return false;
                }
                if (!l3_blocks_equal(lhs.l3()[lhs_cell.ptr()], rhs.l3()[rhs_cell.ptr()]))
                {
                    return false;
                }
            }
            else if (lhs_cell.port() != rhs_cell.port())
            {
                return false;
            }
        }
        return true;
    }

    bool trees_isomorphic(const gen::LuleaTree &lhs, const gen::LuleaTree &rhs)
    {
        if (!configs_equal(lhs.config(), rhs.config()) ||
            lhs.l2().size() != rhs.l2().size() ||
            lhs.l3().size() != rhs.l3().size())
        {
            return false;
        }
        const gen::LuleaRoute &lhs_l1 = lhs.l1();
        const gen::LuleaRoute &rhs_l1 = rhs.l1();
        if (lhs_l1.size() != rhs_l1.size() ||
            lhs_l1.cell_size_bits() != rhs_l1.cell_size_bits())
        {
            return false;
        }
        for (size_t idx = 0; idx < lhs_l1.size(); ++idx)
        {
            const gen::LuleaCell &lhs_cell = lhs_l1[idx];
            const gen::LuleaCell &rhs_cell = rhs_l1[idx];
            if (lhs_cell.is_ptr() != rhs_cell.is_ptr())
            {
                return false;
            }
            if (lhs_cell.is_ptr())
            {
                if (!l2_blocks_equal(lhs, rhs, {lhs_cell.ptr(), rhs_cell.ptr()}))
                {
                    return false;
                }
            }
            else if (lhs_cell.port() != rhs_cell.port())
            {
                return false;
            }
        }
        return true;
    }

    gen::ipv4_t netmask_for_len(uint8_t mask_len)
    {
        if (mask_len == 0)
        {
            return 0u;
        }
        return ~uint32_t{0} << (32u - mask_len);
    }

    gen::MaskEntries make_unique_random_entries(uint32_t seed, size_t count)
    {
        std::mt19937 rng(seed);
        std::set<std::pair<gen::ipv4_t, uint8_t>> seen;
        gen::MaskEntries entries;
        entries.reserve(count);

        entries.push_back(make_entry(0x00000000u, 0, 1));
        seen.emplace(0x00000000u, uint8_t{0});

        while (entries.size() < count)
        {
            const uint8_t len = static_cast<uint8_t>((rng() % 32u) + 1u);
            const uint32_t raw_rand = static_cast<uint32_t>(rng());
            const uint32_t ip = (entries.size() % 2 == 0)
                ? (0xC0A80000u | (raw_rand & 0xFFFFu))
                : raw_rand;
            const uint32_t masked_ip = ip & netmask_for_len(len);
            if (!seen.emplace(masked_ip, len).second)
            {
                continue;
            }
            const uint8_t port = static_cast<uint8_t>(rng() % 256u);
            entries.push_back(make_entry(ip, len, port));
        }
        return entries;
    }

    std::vector<gen::ipv4_t> make_probe_ips(uint32_t seed, size_t count)
    {
        std::mt19937 rng(seed);
        std::vector<gen::ipv4_t> ips;
        ips.reserve(count);
        for (size_t idx = 0; idx < count; ++idx)
        {
            const uint32_t raw_rand = static_cast<uint32_t>(rng());
            const uint32_t ip = (idx % 2 == 0)
                ? (0xC0A80000u | (raw_rand & 0xFFFFu))
                : raw_rand;
            ips.push_back(ip);
        }
        return ips;
    }
} // namespace

TEST(LuleaTreeTest, ThrowsOnInvalidConfig)
{
    EXPECT_THROW(gen::LuleaTree(gen::LuleaTreeConfig{0, 16, 16}), std::invalid_argument);
    EXPECT_THROW(gen::LuleaTree(gen::LuleaTreeConfig{16, 0, 16}), std::invalid_argument);
    EXPECT_THROW(gen::LuleaTree(gen::LuleaTreeConfig{16, 16, 0}), std::invalid_argument);
    EXPECT_THROW(gen::LuleaTree(gen::LuleaTreeConfig{18, 6, 7}), std::invalid_argument);
    EXPECT_THROW(gen::LuleaTree(gen::LuleaTreeConfig{16, 8, 9}), std::invalid_argument);
}

TEST(LuleaTreeTest, InvalidMaskLength)
{
    gen::LuleaTree tree({make_entry(0x0A000000, 8, 5)}, gen::LuleaTreeConfig{16, 8, 8});
    EXPECT_EQ(tree_lookup(tree, 0x0A123456), gen::port_t{5});

    const gen::MaskEntries bad = {make_entry(0xC0A80100, 33, 9)};
    EXPECT_THROW(tree.build(bad), std::invalid_argument);

    // Existing state remains intact after failed build
    EXPECT_EQ(tree_lookup(tree, 0x0A123456), gen::port_t{5});
}

TEST(LuleaTreeTest, EmptyTreeResolvesToNullPort)
{
    const gen::LuleaTree tree;

    EXPECT_EQ(tree.config().l1_bits, gen::L1_IP_SIZE);
    EXPECT_EQ(tree.config().l2_bits, gen::L2_IP_SIZE);
    EXPECT_EQ(tree.config().l3_bits, gen::L3_IP_SIZE);
    EXPECT_EQ(tree.config().l1_cell_bits, gen::L1_DEFAULT_SEQ_CELL);
    EXPECT_EQ(tree.config().l2_cell_bits, gen::L2_DEFAULT_SEQ_CELL);
    EXPECT_EQ(tree.config().l3_cell_bits, gen::L3_DEFAULT_SEQ_CELL);
    EXPECT_EQ(tree.l1().size(), size_t{1} << gen::L1_IP_SIZE);
    EXPECT_TRUE(tree.l2().empty());
    EXPECT_TRUE(tree.l3().empty());

    EXPECT_EQ(tree_lookup(tree, 0x00000000), gen::NULL_PORT);
    EXPECT_EQ(tree_lookup(tree, 0xC0A80101), gen::NULL_PORT);
    EXPECT_EQ(tree_lookup(tree, 0xFFFFFFFF), gen::NULL_PORT);
}

TEST(LuleaTreeTest, DefaultRoutePopulatesL1Only)
{
    const gen::LuleaTree tree(
        {make_entry(0x00000000, 0, 42)},
        gen::LuleaTreeConfig{16, 8, 8});

    EXPECT_TRUE(tree.l2().empty());
    EXPECT_TRUE(tree.l3().empty());
    EXPECT_EQ(tree_lookup(tree, 0x00000000), gen::port_t{42});
    EXPECT_EQ(tree_lookup(tree, 0x7F000001), gen::port_t{42});
    EXPECT_EQ(tree_lookup(tree, 0xFFFFFFFF), gen::port_t{42});
}

TEST(LuleaTreeTest, SchemeHierarchy)
{
    // Models docs/cache_scheme.drawio (base_concept_18_6_8):
    // - A /7 prefix resolving directly in L1
    // - A /21 prefix allocating an L2 block (depth 19..24)
    // - A /30 prefix inside another L1 slot/L2 slot allocating an L3 block (depth 25..32)
    const gen::MaskEntries entries = {
        make_entry(0x0C000000, 7, 10),  // 0000_110... -> port 10 in L1
        make_entry(0x80000000, 21, 1),  // 128.0.0.0/21 -> port 1 in L2
        make_entry(0x80040210, 30, 2),  // 128.4.2.16/30 -> port 2 in L3
    };

    const gen::LuleaTree tree(entries, gen::LuleaTreeConfig{18, 6, 8});

    ASSERT_EQ(tree.l2().size(), 2u);
    ASSERT_EQ(tree.l3().size(), 1u);

    // L1 direct hit
    EXPECT_TRUE(tree.l1()[0x0C000000 >> 14].is_port());
    EXPECT_EQ(tree_lookup(tree, 0x0CABCDEF), gen::port_t{10});

    // L2 hit vs background NULL inside same /18 block
    EXPECT_TRUE(tree.l1()[0x80000000 >> 14].is_ptr());
    EXPECT_EQ(tree_lookup(tree, 0x80000000), gen::port_t{1});
    EXPECT_EQ(tree_lookup(tree, 0x800007FF), gen::port_t{1});
    EXPECT_EQ(tree_lookup(tree, 0x80000800), gen::NULL_PORT);

    // L3 hit vs neighbor outside /30
    EXPECT_TRUE(tree.l1()[0x80040210 >> 14].is_ptr());
    EXPECT_EQ(tree_lookup(tree, 0x80040210), gen::port_t{2});
    EXPECT_EQ(tree_lookup(tree, 0x80040213), gen::port_t{2});
    EXPECT_EQ(tree_lookup(tree, 0x80040214), gen::NULL_PORT);
}

TEST(LuleaTreeTest, LongestPrefixMatch)
{
    // Deliberately unsorted: longest and shortest interleaved
    const gen::MaskEntries entries = {
        make_entry(0xC0A80105, 32, 8),  // 192.168.1.5/32 -> port 8
        make_entry(0xC0A80100, 28, 7),  // 192.168.1.0/28 -> port 7
        make_entry(0xC0A80000, 16, 3),  // 192.168.0.0/16 -> port 3
        make_entry(0x00000000, 0, 1),   // default /0 -> port 1
        make_entry(0xC0A80000, 18, 4),  // 192.168.0.0/18 (exact L1 boundary in 18-6-8) -> port 4
        make_entry(0xC0A80000, 21, 5),  // 192.168.0.0/21 -> port 5
        make_entry(0xC0A80100, 24, 6),  // 192.168.1.0/24 (exact L2 boundary) -> port 6
        make_entry(0xC0000000, 8, 2),   // 192.0.0.0/8 -> port 2
    };

    const std::vector<gen::ipv4_t> probe_ips = {
        0xC0A80105, // hits /32 -> 8
        0xC0A80104, // hits /28 -> 7
        0xC0A8010F, // hits /28 -> 7
        0xC0A80110, // hits /24 -> 6
        0xC0A801FF, // hits /24 -> 6
        0xC0A80201, // hits /21 -> 5
        0xC0A82001, // hits /18 -> 4
        0xC0A84001, // hits /16 -> 3
        0xC0000001, // hits /8  -> 2
        0x01020304, // hits /0  -> 1
    };

    const std::vector<gen::LuleaTreeConfig> configs = {
        gen::LuleaTreeConfig{16, 8, 8},
        gen::LuleaTreeConfig{18, 6, 8},
        gen::LuleaTreeConfig{20, 4, 8},
    };

    for (const auto &cfg : configs)
    {
        const gen::LuleaTree tree(entries, cfg);
        for (const auto ip : probe_ips)
        {
            const auto expected = reference_lpm(entries, ip);
            EXPECT_EQ(tree_lookup(tree, ip), expected);
        }
    }
}

TEST(LuleaTreeTest, DuplicateOverwrite)
{
    const gen::MaskEntries dup_entries = {
        make_entry(0x0A010000, 24, 5),
        make_entry(0x0A010000, 24, 9),
    };
    const gen::LuleaTree dup_tree(dup_entries, gen::LuleaTreeConfig{18, 6, 8});
    EXPECT_EQ(tree_lookup(dup_tree, 0x0A010001), gen::port_t{9});
    EXPECT_EQ(tree_lookup(dup_tree, 0x0A0100FF), gen::port_t{9});

    const gen::MaskEntries dup_host_entries = {
        make_entry(0xC0A801FFu, 24, 11),
        make_entry(0xC0A80100u, 24, 22),
    };
    const gen::LuleaTree dup_host_tree(dup_host_entries);
    EXPECT_EQ(tree_lookup(dup_host_tree, 0xC0A80101u), gen::port_t{22});
    EXPECT_EQ(tree_lookup(dup_host_tree, 0xC0A801FEu), gen::port_t{22});

    // Order independence across L1, L2, and L3 boundaries without collapsing
    const gen::MaskEntries short_first = {
        make_entry(0x0A000000, 8, 2),
        make_entry(0x0A020000, 21, 5),
        make_entry(0x0A020000, 26, 7),
        make_entry(0x0A020010, 32, 11),
    };
    const gen::MaskEntries long_first = {
        make_entry(0x0A020010, 32, 11),
        make_entry(0x0A020000, 26, 7),
        make_entry(0x0A020000, 21, 5),
        make_entry(0x0A000000, 8, 2),
    };

    const gen::LuleaTree t1(short_first, gen::LuleaTreeConfig{18, 6, 8});
    const gen::LuleaTree t2(long_first, gen::LuleaTreeConfig{18, 6, 8});

    EXPECT_FALSE(t1.l2().empty());
    EXPECT_FALSE(t1.l3().empty());
    EXPECT_TRUE(trees_isomorphic(t1, t2));

    for (const gen::ipv4_t ip : {0x0A000001u, 0x0A020000u, 0x0A020010u, 0x0A020011u, 0x0A020040u, 0x0A020800u, 0x0B000001u})
    {
        EXPECT_EQ(tree_lookup(t1, ip), tree_lookup(t2, ip));
        EXPECT_EQ(tree_lookup(t1, ip), reference_lpm(short_first, ip));
    }
}

TEST(LuleaTreeTest, ExplicitNullPort)
{
    const gen::MaskEntries entries = {
        make_entry(0x0A000000, 8, 5),    // 10.0.0.0/8 -> port 5
        make_entry(0x0A00017F, 24, 0),   // 10.0.1.0/24 (host bits set) -> NULL_PORT
        make_entry(0x0A00012A, 32, 9),   // 10.0.1.42/32 -> port 9
    };

    const gen::LuleaTree tree(entries, gen::LuleaTreeConfig{18, 6, 8});

    EXPECT_EQ(tree_lookup(tree, 0x0A000001), gen::port_t{5});
    EXPECT_EQ(tree_lookup(tree, 0x0A000101), gen::NULL_PORT);
    EXPECT_EQ(tree_lookup(tree, 0x0A00017F), gen::NULL_PORT);
    EXPECT_EQ(tree_lookup(tree, 0x0A00012A), gen::port_t{9});
}

TEST(LuleaTreeTest, LuleaOverflow)
{
    gen::MaskEntries entries;
    // 300 distinct /16 prefixes with /24 and /32 inside allocate 300 L2 and 300 L3 blocks.
    for (uint32_t i = 0; i < 300; ++i)
    {
        const uint32_t base = (uint32_t{0x0A} << 24) | (i << 16);
        entries.push_back(make_entry(base, 24, 1));
        entries.push_back(make_entry(base | 0x00000001u, 32, static_cast<uint8_t>(10 + (i % 200))));
    }

    // Default L2_DEFAULT_SEQ_CELL = 9 allows ptr indices 0..255 -> 257th L3 block overflows.
    EXPECT_THROW(
        gen::LuleaTree(entries, gen::LuleaTreeConfig{16, 8, 8}),
        gen::LuleaOverflow);

    // Explicit l2_cell_bits = 10 allows up to 512 L3 blocks without dynamic resizing.
    const gen::LuleaTreeConfig wide_l2{16, 8, 8, gen::L1_DEFAULT_SEQ_CELL, 10, gen::L3_DEFAULT_SEQ_CELL};
    const gen::LuleaTree tree(entries, wide_l2);

    EXPECT_EQ(tree.l2().size(), 300u);
    EXPECT_EQ(tree.l3().size(), 300u);
    EXPECT_EQ(tree.l1().cell_size_bits(), gen::L1_DEFAULT_SEQ_CELL);
    EXPECT_EQ(tree.l2()[0].cell_size_bits(), 10u);

    for (uint32_t i = 0; i < 300; ++i)
    {
        const uint32_t base = (uint32_t{0x0A} << 24) | (i << 16);
        EXPECT_EQ(tree_lookup(tree, base | 0x00000001u), static_cast<gen::port_t>(10 + (i % 200)));
        EXPECT_EQ(tree_lookup(tree, base | 0x00000002u), gen::port_t{1});
        EXPECT_EQ(tree_lookup(tree, base | 0x00000102u), gen::NULL_PORT);
    }

    // Small l1_cell_bits = 2 allows at most 2 L2 blocks (ptr 0 and 1); 3rd L2 block throws LuleaOverflow.
    const gen::MaskEntries three_l2 = {
        make_entry(0x01000000, 20, 1),
        make_entry(0x02000000, 20, 1),
        make_entry(0x03000000, 20, 1),
    };
    const gen::LuleaTreeConfig tiny_l1{18, 6, 8, 2, gen::L2_DEFAULT_SEQ_CELL, gen::L3_DEFAULT_SEQ_CELL};
    EXPECT_THROW(gen::LuleaTree(three_l2, tiny_l1), gen::LuleaOverflow);
}

TEST(LuleaTreeTest, PseudoRandomPrefixes)
{
    std::mt19937 rng(20261111u);
    gen::MaskEntries entries;

    entries.push_back(make_entry(0x00000000, 0, 1));
    for (size_t i = 0; i < 400; ++i)
    {
        const uint8_t mask = static_cast<uint8_t>((rng() % 32u) + 1u);
        // Concentrate half the prefixes inside 192.168.0.0/16 to stress multi-level overlaps
        const uint32_t ip = (i % 2 == 0)
            ? (0xC0A80000u | (rng() & 0xFFFFu))
            : rng();
        const uint8_t port = static_cast<uint8_t>(rng() % 256u);
        entries.push_back(make_entry(ip, mask, port));
    }

    for (const auto &cfg : {gen::LuleaTreeConfig{16, 8, 8}, gen::LuleaTreeConfig{18, 6, 8}})
    {
        const gen::LuleaTree tree(entries, cfg);

        for (const auto &entry : entries)
        {
            const uint32_t ip = entry.ip;
            EXPECT_EQ(tree_lookup(tree, ip), reference_lpm(entries, ip));
            EXPECT_EQ(tree_lookup(tree, ip ^ 1u), reference_lpm(entries, ip ^ 1u));
        }

        for (size_t q = 0; q < 2000; ++q)
        {
            const uint32_t ip = (q % 2 == 0)
                ? (0xC0A80000u | (rng() & 0xFFFFu))
                : rng();
            EXPECT_EQ(tree_lookup(tree, ip), reference_lpm(entries, ip));
        }
    }
}

TEST(LuleaTreeTest, TreeIsomorphism)
{
    const gen::LuleaTreeConfig cfg{18, 6, 8};
    const gen::MaskEntries order_a = {
        make_entry(0x0A000105u, 32, 1),
        make_entry(0x0B000105u, 32, 2),
    };
    const gen::MaskEntries order_b = {
        make_entry(0x0B000105u, 32, 2),
        make_entry(0x0A000105u, 32, 1),
    };

    const gen::LuleaTree tree_a(order_a, cfg);
    const gen::LuleaTree tree_b(order_b, cfg);

    // Block indices in L1/L2 differ by insertion order, yet the trees are isomorphic.
    EXPECT_NE(tree_a.l1()[0x0A000105u >> 14].ptr(), tree_b.l1()[0x0A000105u >> 14].ptr());
    EXPECT_TRUE(trees_isomorphic(tree_a, tree_b));

    const gen::LuleaTree diff_port(
        {make_entry(0x0A000105u, 32, 1), make_entry(0x0B000105u, 32, 9)},
        cfg);
    EXPECT_FALSE(trees_isomorphic(tree_a, diff_port));

    const gen::LuleaTree diff_struct({make_entry(0x0A000105u, 32, 1)}, cfg);
    EXPECT_FALSE(trees_isomorphic(tree_a, diff_struct));

    const gen::LuleaTree diff_cfg(order_a, gen::LuleaTreeConfig{16, 8, 8});
    EXPECT_FALSE(trees_isomorphic(tree_a, diff_cfg));
}

TEST(LuleaTreeTest, ShuffledPrefixes)
{
    const gen::MaskEntries entries = make_unique_random_entries(20261112u, 300);
    const std::vector<gen::ipv4_t> probes = make_probe_ips(99887766u, 1000);

    std::vector<gen::port_t> expected(probes.size(), gen::NULL_PORT);
    for (size_t idx = 0; idx < probes.size(); ++idx)
    {
        expected[idx] = reference_lpm(entries, probes[idx]);
    }

    const std::vector<gen::LuleaTreeConfig> configs = {
        gen::LuleaTreeConfig{16, 8, 8},
        gen::LuleaTreeConfig{18, 6, 8},
    };

    std::mt19937 shuffle_rng(13579u);
    gen::MaskEntries shuffled = entries;

    for (const gen::LuleaTreeConfig &cfg : configs)
    {
        const gen::LuleaTree baseline(entries, cfg);
        for (size_t idx = 0; idx < probes.size(); ++idx)
        {
            EXPECT_EQ(tree_lookup(baseline, probes[idx]), expected[idx]);
        }

        for (size_t perm = 0; perm < 8; ++perm)
        {
            std::shuffle(shuffled.begin(), shuffled.end(), shuffle_rng);
            const gen::LuleaTree permuted(shuffled, cfg);

            EXPECT_TRUE(trees_isomorphic(baseline, permuted));
            for (size_t idx = 0; idx < probes.size(); ++idx)
            {
                EXPECT_EQ(tree_lookup(permuted, probes[idx]), expected[idx]);
            }
        }
    }
}

TEST(LuleaTreeTest, PreserveBlocks)
{
    const gen::MaskEntries entries = {
        make_entry(0x0A020000u, 25, 4),
        make_entry(0x0A020080u, 25, 4),
    };
    const gen::LuleaTree tree(entries, gen::LuleaTreeConfig{18, 6, 8});

    EXPECT_EQ(tree.l2().size(), 1u);
    EXPECT_EQ(tree.l3().size(), 1u);
    EXPECT_EQ(tree_lookup(tree, 0x0A020010u), gen::port_t{4});
    EXPECT_EQ(tree_lookup(tree, 0x0A020090u), gen::port_t{4});
}

TEST(LuleaTreeTest, LazySplit)
{
    const gen::MaskEntries entries = {
        make_entry(0x0A000000u, 8, 5),
        make_entry(0x0A010203u, 32, 9),
        make_entry(0x0A010000u, 16, 3),
    };
    const gen::LuleaTree tree(entries, gen::LuleaTreeConfig{18, 6, 8});

    EXPECT_EQ(tree.l2().size(), 1u);
    EXPECT_EQ(tree.l3().size(), 1u);
    EXPECT_EQ(tree_lookup(tree, 0x0A000001u), gen::port_t{5});
    EXPECT_EQ(tree_lookup(tree, 0x0A010001u), gen::port_t{3});
    EXPECT_EQ(tree_lookup(tree, 0x0A010204u), gen::port_t{3});
    EXPECT_EQ(tree_lookup(tree, 0x0A010203u), gen::port_t{9});
    EXPECT_EQ(tree_lookup(tree, 0x0B000001u), gen::NULL_PORT);
}

