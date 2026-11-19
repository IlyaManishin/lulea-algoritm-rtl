#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

#include "config.hpp"
#include "engine/lulea_cache_lookup.hpp"
#include "engine/lulea_tree_lookup.hpp"
#include "gen.hpp"
#include "io/reader.hpp"
#include "lulea_algo/lulea_algo.hpp"
#include "lulea_algo/lulea_tree.hpp"
#include "utils/ip_gen.hpp"
#include "utils/linear_lookup.hpp"
#include "utils/utils.hpp"

namespace fs = std::filesystem;

class LuleaCacheRealTableTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = fs::temp_directory_path() / "lulea_cache_real_tests";
        fs::create_directories(test_dir_);
    }

    void TearDown() override
    {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
};

TEST_F(LuleaCacheRealTableTest, VerifyCachesWithReference)
{
    const auto tables = tests::get_test_tables();
    ASSERT_FALSE(tables.empty()) << "No routing tables found";

    size_t tables_tested = 0;

    for (const auto &table : tables)
    {
        const gen::MaskEntries bin_entries = gen::Reader::read(table.bin_path.string());
        EXPECT_EQ(bin_entries.size(), 2048U);

        const gen::MaskEntries txt_entries = tests::LinearLookup::parse_routes_txt(table.txt_path.string());
        EXPECT_EQ(txt_entries.size(), bin_entries.size());

        for (size_t idx = 0; idx < txt_entries.size(); ++idx)
        {
            EXPECT_EQ(bin_entries[idx].ip, txt_entries[idx].ip);
            EXPECT_EQ(bin_entries[idx].mask, txt_entries[idx].mask);
            EXPECT_EQ(bin_entries[idx].port, txt_entries[idx].port);
        }

        const gen::LuleaTree tree(bin_entries);
        const fs::path dump_dir = test_dir_ / table.name;
        const gen::Config config{
            .mode = gen::Mode::Dump,
            .input_path = table.bin_path.string(),
            .dest_dir = dump_dir.string(),
        };

        gen::LuleaAlgo algo(tree, config);
        algo.generate_caches(true);

        const engine::LuleaCacheLookup cache_lookup(dump_dir);
        const tests::LinearLookup linear_lookup(txt_entries);
        const tests::IpGen ip_gen(bin_entries);

        const auto edge_ips = ip_gen.ip_gen_edge();
        const auto edge_res = tests::verify_lookups(cache_lookup, linear_lookup, edge_ips);
        EXPECT_TRUE(edge_res.is_success())
            << "Edge verification failed on " << table.name
            << " (" << edge_res.mismatched << "/" << edge_res.total << " mismatches)";

        const auto random_ips = ip_gen.ip_gen_random(2000, static_cast<uint32_t>(1000 + tables_tested));
        const auto rand_res = tests::verify_lookups(cache_lookup, linear_lookup, random_ips);
        EXPECT_TRUE(rand_res.is_success())
            << "Random verification failed on " << table.name
            << " (" << rand_res.mismatched << "/" << rand_res.total << " mismatches)";

        const auto step_ips = ip_gen.ip_gen_step(0x01000000U, 131072U, 1000);
        const auto step_res = tests::verify_lookups(cache_lookup, linear_lookup, step_ips);
        EXPECT_TRUE(step_res.is_success())
            << "Step verification failed on " << table.name
            << " (" << step_res.mismatched << "/" << step_res.total << " mismatches)";

        tables_tested++;
    }

    EXPECT_EQ(tables_tested, tables.size());
}
