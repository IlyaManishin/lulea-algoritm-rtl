#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "engine/lulea_tree_lookup.hpp"
#include "io/reader.hpp"
#include "lulea_algo/lulea_tree.hpp"
#include "utils/ip_gen.hpp"
#include "utils/linear_lookup.hpp"
#include "utils/utils.hpp"

TEST(LuleaTreeRealTableTest, VerifyTreeWithReference)
{
    const auto tables = tests::get_test_tables();
    ASSERT_FALSE(tables.empty()) << "No routing tables found";

    size_t tables_tested = 0;

    for (const auto &table : tables)
    {
        const gen::MaskEntries bin_entries = gen::Reader::read(table.bin_path.string());
        const gen::MaskEntries txt_entries = tests::LinearLookup::parse_routes_txt(table.txt_path.string());

        const gen::LuleaTree tree(bin_entries);
        const engine::LuleaTreeLookup tree_lookup(tree);
        const tests::LinearLookup linear_lookup(txt_entries);
        const tests::IpGen ip_gen(bin_entries);

        const auto edge_ips = ip_gen.ip_gen_edge();
        const auto edge_res = tests::verify_lookups(tree_lookup, linear_lookup, edge_ips);
        EXPECT_TRUE(edge_res.is_success())
            << "Edge verification failed on " << table.name
            << " (" << edge_res.mismatched << "/" << edge_res.total << " mismatches)";

        const auto seed = static_cast<uint32_t>(1000 + tables_tested);
        const auto random_ips = tests::IpGen::ip_gen_random(tests::REAL_TABLE_RANDOM_IP_COUNT, seed);
        const auto rand_res = tests::verify_lookups(tree_lookup, linear_lookup, random_ips);
        EXPECT_TRUE(rand_res.is_success())
            << "Random verification failed on " << table.name
            << " (" << rand_res.mismatched << "/" << rand_res.total << " mismatches)";

        const auto step_ips = tests::IpGen::ip_gen_step(0U, tests::REAL_TABLE_IP_STEP, tests::REAL_TABLE_STEP_IP_COUNT);
        const auto step_res = tests::verify_lookups(tree_lookup, linear_lookup, step_ips);
        EXPECT_TRUE(step_res.is_success())
            << "Step verification failed on " << table.name
            << " (" << step_res.mismatched << "/" << step_res.total << " mismatches)";

        tables_tested++;
    }

    EXPECT_EQ(tables_tested, tables.size());
}
