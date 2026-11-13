#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "config.hpp"
#include "core/lulea_tree.hpp"
#include "io/reader.hpp"
#include "ip_gen.hpp"
#include "reference_lpm.hpp"

namespace fs = std::filesystem;

namespace
{

    fs::path get_tables_root_dir()
    {
        const fs::path source_dir = fs::path(__FILE__).parent_path();
        const fs::path tables_dir = source_dir / "tables";
        if (fs::is_directory(tables_dir))
        {
            return tables_dir;
        }

        // Fallback relative paths
        const std::vector<fs::path> candidates = {
            "tests/lulea/tables",
            "../tests/lulea/tables",
            "cache_gen/tests/lulea/tables",
        };
        const auto it = std::find_if(candidates.begin(), candidates.end(), [](const fs::path &cand) {
            return fs::is_directory(cand);
        });
        if (it != candidates.end())
        {
            return *it;
        }
        return tables_dir;
    }

} // namespace

TEST(IpGenTest, GeneratesDeterministicRandomIps)
{
    const gen::MaskEntries entries = {
        gen::TMaskEntry{.ip = 0x0A000000u, .mask = gen::mask_t{8}, .port = gen::port_t{1}},
    };

    const gen::IpGen gen(entries);
    const auto ips1 = gen.ip_gen_random(100, 42);
    const auto ips2 = gen.ip_gen_random(100, 42);
    const auto ips_diff = gen.ip_gen_random(100, 43);

    EXPECT_EQ(ips1.size(), 100u);
    EXPECT_EQ(ips1, ips2);
    EXPECT_NE(ips1, ips_diff);
}

TEST(IpGenTest, GeneratesStepProgression)
{
    const gen::MaskEntries entries;
    const gen::IpGen gen(entries);

    const auto ips = gen.ip_gen_step(10, 5, 4);
    const std::vector<gen::ipv4_t> expected = {10, 15, 20, 25};
    EXPECT_EQ(ips, expected);
}

TEST(IpGenTest, GeneratesSubnetEdgeCasesAndBoundaries)
{
    const gen::MaskEntries entries = {
        gen::TMaskEntry{.ip = 0xC0A80100u, .mask = gen::mask_t{24}, .port = gen::port_t{1}}, // 192.168.1.0/24
    };

    const gen::IpGen gen(entries);
    const auto edge_ips = gen.ip_gen_edge();

    EXPECT_FALSE(edge_ips.empty());

    // Must contain base, last, center, base-1, last+1
    auto contains_ip = [&edge_ips](gen::ipv4_t target) {
        return std::binary_search(edge_ips.begin(), edge_ips.end(), target);
    };

    EXPECT_TRUE(contains_ip(0xC0A80100u));        // base
    EXPECT_TRUE(contains_ip(0xC0A801FFu));        // last
    EXPECT_TRUE(contains_ip(0xC0A80100u - 1u));   // outside lower
    EXPECT_TRUE(contains_ip(0xC0A801FFu + 1u));   // outside upper
    EXPECT_TRUE(contains_ip(0xC0A80100u + 128u)); // center
}

TEST(ReferenceLpmTest, MatchesLongestPrefixAndHandlesDefaultRoute)
{
    const gen::MaskEntries entries = {
        gen::TMaskEntry{.ip = 0x00000000u, .mask = gen::mask_t{0}, .port = gen::port_t{10}},
        gen::TMaskEntry{.ip = 0x0A000000u, .mask = gen::mask_t{8}, .port = gen::port_t{1}},
        gen::TMaskEntry{.ip = 0x0A010000u, .mask = gen::mask_t{16}, .port = gen::port_t{2}},
        gen::TMaskEntry{.ip = 0x0A010200u, .mask = gen::mask_t{24}, .port = gen::port_t{3}},
    };

    const gen::ReferenceLpm ref(entries);

    // /24 match
    EXPECT_EQ(ref.lookup(0x0A010205u), gen::port_t{3});
    // /16 match
    EXPECT_EQ(ref.lookup(0x0A010305u), gen::port_t{2});
    // /8 match
    EXPECT_EQ(ref.lookup(0x0A050505u), gen::port_t{1});
    // default route match
    EXPECT_EQ(ref.lookup(0xC0A80101u), gen::port_t{10});
}

TEST(LuleaTreeRealTableTest, VerifiesTreeAgainstReferenceOnAllTenRealTables)
{
    const fs::path tables_root = get_tables_root_dir();
    ASSERT_TRUE(fs::is_directory(tables_root)) << "Tables directory not found: " << tables_root;

    size_t tables_tested = 0;

    for (size_t i = 1; i <= 10; ++i)
    {
        const fs::path table_dir = tables_root / ("table_" + std::to_string(i));
        if (!fs::is_directory(table_dir))
        {
            continue;
        }

        const fs::path bin_path = table_dir / "routes.bin";
        const fs::path txt_path = table_dir / "routes.txt";

        ASSERT_TRUE(fs::is_regular_file(bin_path)) << "Missing " << bin_path;
        ASSERT_TRUE(fs::is_regular_file(txt_path)) << "Missing " << txt_path;

        // 1. Read binary mapping for LuleaTree
        std::unique_ptr<gen::MaskEntries> bin_entries(gen::read_mask_mapping(bin_path.string()));
        ASSERT_NE(bin_entries, nullptr);
        EXPECT_EQ(bin_entries->size(), 2048u);

        // 2. Read text routes for Reference LPM
        const gen::MaskEntries txt_entries = gen::ReferenceLpm::parse_routes_txt(txt_path.string());
        EXPECT_EQ(txt_entries.size(), bin_entries->size());

        // Verify binary and text entries are identical
        for (size_t idx = 0; idx < txt_entries.size(); ++idx)
        {
            EXPECT_EQ((*bin_entries)[idx].ip, txt_entries[idx].ip);
            EXPECT_EQ((*bin_entries)[idx].mask, txt_entries[idx].mask);
            EXPECT_EQ((*bin_entries)[idx].port, txt_entries[idx].port);
        }

        // 3. Build LuleaTree and ReferenceLpm
        gen::LuleaTree tree(*bin_entries);
        const gen::ReferenceLpm reference(txt_entries);
        const gen::IpGen ip_gen(*bin_entries);

        // 4. Verify Edge Cases (boundary testing)
        const auto edge_ips = ip_gen.ip_gen_edge();
        const auto edge_res = gen::verify_lookups(tree, reference, edge_ips);
        EXPECT_TRUE(edge_res.is_success())
            << "Edge verification failed on table_" << i
            << " (" << edge_res.mismatched << "/" << edge_res.total << " mismatches)";

        // 5. Verify Random IPs (pseudo-random sample)
        const auto random_ips = ip_gen.ip_gen_random(2000, static_cast<uint32_t>(1000 + i));
        const auto rand_res = gen::verify_lookups(tree, reference, random_ips);
        EXPECT_TRUE(rand_res.is_success())
            << "Random verification failed on table_" << i
            << " (" << rand_res.mismatched << "/" << rand_res.total << " mismatches)";

        // 6. Verify Stepped IPs
        const auto step_ips = ip_gen.ip_gen_step(0x01000000u, 131072u, 1000);
        const auto step_res = gen::verify_lookups(tree, reference, step_ips);
        EXPECT_TRUE(step_res.is_success())
            << "Step verification failed on table_" << i
            << " (" << step_res.mismatched << "/" << step_res.total << " mismatches)";

        tables_tested++;
    }

    EXPECT_EQ(tables_tested, 10u);
}
