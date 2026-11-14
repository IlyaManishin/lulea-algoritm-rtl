#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "config.hpp"
#include "core/lulea_tree.hpp"
#include "engine/lulea_tree_lookup.hpp"
#include "io/reader.hpp"
#include "utils/ip_gen.hpp"
#include "utils/linear_lookup.hpp"
#include "utils/test_env.hpp"

namespace fs = std::filesystem;

TEST(LuleaTreeRealTableTest, VerifyTreeWithReference)
{
    const fs::path tables_root = tests::get_tables_root_dir();
    ASSERT_TRUE(fs::is_directory(tables_root)) << "Tables directory not found: " << tables_root;

    size_t tables_tested = 0;

    for (size_t table_idx = 1; table_idx <= 10; ++table_idx)
    {
        const fs::path table_dir = tables_root / ("table_" + std::to_string(table_idx));
        if (!fs::is_directory(table_dir))
        {
            continue;
        }

        const fs::path bin_path = table_dir / "routes.bin";
        const fs::path txt_path = table_dir / "routes.txt";

        ASSERT_TRUE(fs::is_regular_file(bin_path)) << "Missing " << bin_path;
        ASSERT_TRUE(fs::is_regular_file(txt_path)) << "Missing " << txt_path;

        // 1. Read binary mapping for LuleaTree
        const gen::MaskEntries bin_entries = gen::Reader::read(bin_path.string());
        EXPECT_EQ(bin_entries.size(), 2048U);

        // 2. Read text routes for LinearLookup
        const gen::MaskEntries txt_entries = tests::LinearLookup::parse_routes_txt(txt_path.string());
        EXPECT_EQ(txt_entries.size(), bin_entries.size());

        // Verify binary and text entries are identical
        for (size_t idx = 0; idx < txt_entries.size(); ++idx)
        {
            EXPECT_EQ(bin_entries[idx].ip, txt_entries[idx].ip);
            EXPECT_EQ(bin_entries[idx].mask, txt_entries[idx].mask);
            EXPECT_EQ(bin_entries[idx].port, txt_entries[idx].port);
        }

        // 3. Build LuleaTree, LuleaTreeLookup and LinearLookup
        const gen::LuleaTree tree(bin_entries);
        const engine::LuleaTreeLookup tree_lookup(tree);
        const tests::LinearLookup linear_lookup(txt_entries);
        const tests::IpGen ip_gen(bin_entries);

        // 4. Verify Edge Cases (boundary testing)
        const auto edge_ips = ip_gen.ip_gen_edge();
        const auto edge_res = tests::verify_lookups(tree_lookup, linear_lookup, edge_ips);
        EXPECT_TRUE(edge_res.is_success())
            << "Edge verification failed on table_" << table_idx
            << " (" << edge_res.mismatched << "/" << edge_res.total << " mismatches)";

        // 5. Verify Random IPs (pseudo-random sample)
        const auto random_ips = ip_gen.ip_gen_random(2000, static_cast<uint32_t>(1000 + table_idx));
        const auto rand_res = tests::verify_lookups(tree_lookup, linear_lookup, random_ips);
        EXPECT_TRUE(rand_res.is_success())
            << "Random verification failed on table_" << table_idx
            << " (" << rand_res.mismatched << "/" << rand_res.total << " mismatches)";

        // 6. Verify Stepped IPs
        const auto step_ips = ip_gen.ip_gen_step(0x01000000U, 131072U, 1000);
        const auto step_res = tests::verify_lookups(tree_lookup, linear_lookup, step_ips);
        EXPECT_TRUE(step_res.is_success())
            << "Step verification failed on table_" << table_idx
            << " (" << step_res.mismatched << "/" << step_res.total << " mismatches)";

        tables_tested++;
    }

    EXPECT_EQ(tables_tested, 10U);
}
