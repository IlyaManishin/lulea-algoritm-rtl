#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>

#include "engine/lulea_cache_lookup.hpp"
#include "gen.hpp"
#include "io/reader.hpp"
#include "utils/ip_gen.hpp"
#include "utils/linear_lookup.hpp"
#include "utils/utils.hpp"

namespace fs = std::filesystem;

namespace
{
    void expect_same_routes(const gen::MaskEntries &bin_entries, const gen::MaskEntries &txt_entries)
    {
        for (size_t idx = 0; idx < txt_entries.size(); ++idx)
        {
            EXPECT_EQ(bin_entries[idx].ip, txt_entries[idx].ip);
            EXPECT_EQ(bin_entries[idx].mask, txt_entries[idx].mask);
            EXPECT_EQ(bin_entries[idx].port, txt_entries[idx].port);
        }
    }

    void verify_table_caches(const tests::TableTestFiles &table, const fs::path &test_dir, uint32_t seed)
    {
        SCOPED_TRACE(table.name);
        const gen::MaskEntries bin_entries = gen::Reader::read(table.bin_path.string());
        const gen::MaskEntries txt_entries = tests::LinearLookup::parse_routes_txt(table.txt_path.string());
        ASSERT_EQ(txt_entries.size(), bin_entries.size());

        expect_same_routes(bin_entries, txt_entries);

        const fs::path dump_dir = test_dir / table.name;
        const gen::Config config{
            .mode = gen::Mode::Dump,
            .input_path = table.bin_path.string(),
            .dest_dir = dump_dir.string(),
        };

        gen::build_lulea_cache(config);

        const engine::LuleaCacheLookup cache_lookup(dump_dir);
        const tests::LinearLookup linear_lookup(txt_entries);
        const tests::IpGen ip_gen(bin_entries);

        const auto edge_ips = ip_gen.ip_gen_edge();
        const auto edge_res = tests::verify_lookups(cache_lookup, linear_lookup, edge_ips);
        EXPECT_TRUE(edge_res.is_success())
            << "Edge verification failed (" << edge_res.mismatched << "/" << edge_res.total << " mismatches)";

        const auto random_ips = tests::IpGen::ip_gen_random(tests::REAL_TABLE_RANDOM_IP_COUNT, seed);
        const auto rand_res = tests::verify_lookups(cache_lookup, linear_lookup, random_ips);
        EXPECT_TRUE(rand_res.is_success())
            << "Random verification failed (" << rand_res.mismatched << "/" << rand_res.total << " mismatches)";

        const auto step_ips = tests::IpGen::ip_gen_step(0U, tests::REAL_TABLE_IP_STEP, tests::REAL_TABLE_STEP_IP_COUNT);
        const auto step_res = tests::verify_lookups(cache_lookup, linear_lookup, step_ips);
        EXPECT_TRUE(step_res.is_success())
            << "Step verification failed (" << step_res.mismatched << "/" << step_res.total << " mismatches)";
    }
} // namespace

TEST(LuleaCacheRealTableTest, VerifyCachesWithReference)
{
    const auto tables = tests::get_test_tables();
    ASSERT_FALSE(tables.empty()) << "No routing tables found";

    const fs::path test_dir = fs::temp_directory_path() / "lulea_cache_real_tests";
    size_t tables_tested = 0;

    for (const auto &table : tables)
    {
        const auto seed = static_cast<uint32_t>(1000 + tables_tested);
        ASSERT_NO_FATAL_FAILURE(verify_table_caches(table, test_dir, seed));
        tables_tested++;
    }

    fs::remove_all(test_dir);
    EXPECT_EQ(tables_tested, tables.size());
}
