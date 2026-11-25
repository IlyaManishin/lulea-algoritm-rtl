#include <lulea_verify/lookup_verification.hpp>

#include "ip_gen.hpp"
#include "linear_lookup.hpp"
#include "utils.hpp"
#include <lulea_common/reader.hpp>
#include <lulea_verify/lookup_engine.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace tests
{
    namespace
    {
        void verify_engine_with_table(const engine::ILookupEngine &lookup,
                                      const TableTestFiles &table,
                                      const gen::MaskEntries &bin_entries,
                                      uint32_t seed)
        {
            const gen::MaskEntries txt_entries = LinearLookup::parse_routes_txt(table.txt_path.string());
            const LinearLookup linear_lookup(txt_entries);
            const IpGen ip_gen(bin_entries);

            const auto edge_ips = ip_gen.ip_gen_edge();
            const auto edge_res = verify_lookups(lookup, linear_lookup, edge_ips);
            EXPECT_TRUE(edge_res.is_success())
                << "Edge verification failed (" << edge_res.mismatched << "/" << edge_res.total << " mismatches)";

            const auto random_ips = IpGen::ip_gen_random(REAL_TABLE_RANDOM_IP_COUNT, seed);
            const auto rand_res = verify_lookups(lookup, linear_lookup, random_ips);
            EXPECT_TRUE(rand_res.is_success())
                << "Random verification failed (" << rand_res.mismatched << "/" << rand_res.total << " mismatches)";

            const auto step_ips = IpGen::ip_gen_step(0U, REAL_TABLE_IP_STEP, REAL_TABLE_STEP_IP_COUNT);
            const auto step_res = verify_lookups(lookup, linear_lookup, step_ips);
            EXPECT_TRUE(step_res.is_success())
                << "Step verification failed (" << step_res.mismatched << "/" << step_res.total << " mismatches)";
        }
    } // namespace

    void verify_lookup_engine(engine::ILookupEngineFactory &engine_factory)
    {
        const auto tables = get_test_tables();

        uint32_t seed = 1000;
        for (const auto &table : tables)
        {
            SCOPED_TRACE(table.name);
            const gen::MaskEntries bin_entries = gen::Reader::read(table.bin_path.string());
            const auto lookup = engine_factory.get_lookup_engine(table.bin_path, bin_entries);
            ASSERT_NE(lookup, nullptr);
            verify_engine_with_table(*lookup, table, bin_entries, seed);
            ++seed;
        }
    }
} // namespace tests
