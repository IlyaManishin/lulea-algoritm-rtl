#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <vector>

#include "gen/config.hpp"
#include "gen/gen.hpp"
#include "gen/lulea_algo/cache_set.hpp"
#include "gen/lulea_algo/lulea_algo.hpp"
#include "gen/lulea_algo/lulea_tree.hpp"

namespace fs = std::filesystem;

class CacheSetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir = fs::temp_directory_path() / "lulea_cache_set_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override
    {
        fs::remove_all(test_dir);
    }

    fs::path test_dir;
};

TEST_F(CacheSetTest, StatsOnlyDoesNotCreateFilesAndCountsBits)
{
    gen::CacheSet caches(test_dir, false);
    EXPECT_FALSE(caches.write_files());

    const std::vector<std::byte> payload = {
        std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};

    caches.l1_pop().write_bits(0b101, 3);
    caches.l1_chunk().write_bits(0xABCD, 10);
    caches.l1_seq().write_bytes(payload, 11);

    caches.l2_pop().write_bits(1, 1);
    caches.l2_chunk().write_bits(0xFF, 8);
    caches.l2_seq().write_bits(0x1234, 16);

    caches.l3_pop().write_bits(0x7F, 7);
    caches.l3_chunk().write_bytes(payload, 14);
    caches.l3_seq().write_bits(0x42, 8);

    caches.flush();
    caches.close();

    EXPECT_TRUE(fs::is_empty(test_dir));

    const gen::CacheStats stats = caches.collect_stats();
    EXPECT_EQ(stats.cache_sizes.l1_pop, 1U);
    EXPECT_EQ(stats.cache_sizes.l1_chunk, 2U);
    EXPECT_EQ(stats.cache_sizes.l1_seq, 2U);
    EXPECT_EQ(stats.cache_sizes.l2_pop, 1U);
    EXPECT_EQ(stats.cache_sizes.l2_chunk, 1U);
    EXPECT_EQ(stats.cache_sizes.l2_seq, 2U);
    EXPECT_EQ(stats.cache_sizes.l3_pop, 1U);
    EXPECT_EQ(stats.cache_sizes.l3_chunk, 2U);
    EXPECT_EQ(stats.cache_sizes.l3_seq, 1U);
}

TEST_F(CacheSetTest, WriteFilesCreatesAllNineFilesWithMatchingSizes)
{
    const fs::path dump_dir = test_dir / "dump";
    gen::CacheSet caches(dump_dir);
    EXPECT_TRUE(caches.write_files());

    const std::vector<std::byte> payload = {
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};

    caches.l1_pop().write_bits(0x5, 3);
    caches.l1_chunk().write_bits(0x123, 10);
    caches.l1_seq().write_bytes(payload, 11);

    caches.l2_pop().write_bits(1, 1);
    caches.l2_chunk().write_bits(0xAA, 8);
    caches.l2_seq().write_bits(0x5678, 16);

    caches.l3_pop().write_bits(0x3F, 7);
    caches.l3_chunk().write_bytes(payload, 14);
    caches.l3_seq().write_bits(0x99, 8);

    caches.close();

    const gen::CacheStats stats = caches.collect_stats();

    const std::vector<std::pair<std::string, uint32_t>> expected_files = {
        {"l1_pop.bin", stats.cache_sizes.l1_pop},
        {"l1_chunk.bin", stats.cache_sizes.l1_chunk},
        {"l1_seq.bin", stats.cache_sizes.l1_seq},
        {"l2_pop.bin", stats.cache_sizes.l2_pop},
        {"l2_chunk.bin", stats.cache_sizes.l2_chunk},
        {"l2_seq.bin", stats.cache_sizes.l2_seq},
        {"l3_pop.bin", stats.cache_sizes.l3_pop},
        {"l3_chunk.bin", stats.cache_sizes.l3_chunk},
        {"l3_seq.bin", stats.cache_sizes.l3_seq},
    };

    for (const auto &[filename, expected_bytes] : expected_files)
    {
        const fs::path file_path = dump_dir / filename;
        ASSERT_TRUE(fs::exists(file_path)) << "Missing expected file: " << filename;
        EXPECT_EQ(fs::file_size(file_path), expected_bytes) << "Size mismatch for: " << filename;
    }
}

TEST_F(CacheSetTest, StatsAndWriteFilesProduceIdenticalStats)
{
    const fs::path dump_dir = test_dir / "dump_compare";
    gen::CacheSet stats_caches(dump_dir, false);
    gen::CacheSet write_caches(dump_dir, true);

    const std::vector<std::byte> payload = {
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD}};

    for (size_t lvl = 1; lvl <= 3; ++lvl)
    {
        auto &s_writers = stats_caches.level(lvl);
        auto &w_writers = write_caches.level(lvl);

        s_writers.pop.write_bits(lvl, lvl * 3);
        w_writers.pop.write_bits(lvl, lvl * 3);

        s_writers.chunk.write_bits(lvl * 10, lvl * 5);
        w_writers.chunk.write_bits(lvl * 10, lvl * 5);

        s_writers.seq.write_bytes(payload, lvl * 9);
        w_writers.seq.write_bytes(payload, lvl * 9);
    }

    stats_caches.close();
    write_caches.close();

    const gen::CacheStats s_stats = stats_caches.collect_stats();
    const gen::CacheStats w_stats = write_caches.collect_stats();

    EXPECT_EQ(s_stats.cache_sizes.l1_pop, w_stats.cache_sizes.l1_pop);
    EXPECT_EQ(s_stats.cache_sizes.l1_chunk, w_stats.cache_sizes.l1_chunk);
    EXPECT_EQ(s_stats.cache_sizes.l1_seq, w_stats.cache_sizes.l1_seq);

    EXPECT_EQ(s_stats.cache_sizes.l2_pop, w_stats.cache_sizes.l2_pop);
    EXPECT_EQ(s_stats.cache_sizes.l2_chunk, w_stats.cache_sizes.l2_chunk);
    EXPECT_EQ(s_stats.cache_sizes.l2_seq, w_stats.cache_sizes.l2_seq);

    EXPECT_EQ(s_stats.cache_sizes.l3_pop, w_stats.cache_sizes.l3_pop);
    EXPECT_EQ(s_stats.cache_sizes.l3_chunk, w_stats.cache_sizes.l3_chunk);
    EXPECT_EQ(s_stats.cache_sizes.l3_seq, w_stats.cache_sizes.l3_seq);
}

TEST_F(CacheSetTest, LevelAccessorThrowsOnInvalidIndex)
{
    gen::CacheSet caches(false);
    EXPECT_THROW(caches.level(0), std::out_of_range);
    EXPECT_THROW(caches.level(4), std::out_of_range);

    EXPECT_NO_THROW(caches.level(1));
    EXPECT_NO_THROW(caches.level(2));
    EXPECT_NO_THROW(caches.level(3));

    EXPECT_EQ(&caches.level(1), &caches.l1());
    EXPECT_EQ(&caches.level(2), &caches.l2());
    EXPECT_EQ(&caches.level(3), &caches.l3());

    const auto &const_caches = caches;
    EXPECT_THROW(const_caches.level(0), std::out_of_range);
    EXPECT_THROW(const_caches.level(4), std::out_of_range);
    EXPECT_NO_THROW(const_caches.level(1));
    EXPECT_NO_THROW(const_caches.level(2));
    EXPECT_NO_THROW(const_caches.level(3));

    EXPECT_EQ(&const_caches.level(1), &const_caches.l1());
    EXPECT_EQ(&const_caches.level(2), &const_caches.l2());
    EXPECT_EQ(&const_caches.level(3), &const_caches.l3());
}

TEST_F(CacheSetTest, LuleaAlgoMatchesStatsBetweenModes)
{
    const gen::MaskEntries entries = {
        {0xC0A80100, static_cast<gen::mask_t>(24), static_cast<gen::port_t>(1)},
        {0x0A000000, static_cast<gen::mask_t>(8), static_cast<gen::port_t>(2)}
    };

    gen::LuleaTree tree(entries);
    const fs::path dump_dir = test_dir / "algo_dump";
    const gen::Config config{gen::Mode::Dump, "sample.bin", dump_dir.string()};

    gen::LuleaAlgo algo(tree, config);
    const gen::CacheStats stats_only = algo.generate_caches(false);
    const gen::CacheStats write_files = algo.generate_caches();

    EXPECT_EQ(stats_only.cache_sizes.l1_pop, write_files.cache_sizes.l1_pop);
    EXPECT_EQ(stats_only.cache_sizes.l1_chunk, write_files.cache_sizes.l1_chunk);
    EXPECT_EQ(stats_only.cache_sizes.l1_seq, write_files.cache_sizes.l1_seq);

    EXPECT_EQ(stats_only.cache_sizes.l2_pop, write_files.cache_sizes.l2_pop);
    EXPECT_EQ(stats_only.cache_sizes.l2_chunk, write_files.cache_sizes.l2_chunk);
    EXPECT_EQ(stats_only.cache_sizes.l2_seq, write_files.cache_sizes.l2_seq);

    EXPECT_EQ(stats_only.cache_sizes.l3_pop, write_files.cache_sizes.l3_pop);
    EXPECT_EQ(stats_only.cache_sizes.l3_chunk, write_files.cache_sizes.l3_chunk);
    EXPECT_EQ(stats_only.cache_sizes.l3_seq, write_files.cache_sizes.l3_seq);
}
