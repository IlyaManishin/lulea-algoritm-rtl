#include <gtest/gtest.h>

#include <cstddef>

#include "gen/utils/rtl.hpp"

TEST(LuleaConfigTest, ComputesClog2AndBramCellAlign)
{
    EXPECT_EQ(gen::align_to_bram_cell(0), 0u);
    EXPECT_EQ(gen::align_to_bram_cell(1), 9u);
    EXPECT_EQ(gen::align_to_bram_cell(8), 9u);
    EXPECT_EQ(gen::align_to_bram_cell(9), 9u);
    EXPECT_EQ(gen::align_to_bram_cell(10), 18u);
    EXPECT_EQ(gen::align_to_bram_cell(19), 27u);
}

TEST(LuleaConfigTest, ComputesSeqCell)
{
    EXPECT_EQ(gen::calc_raw_seq_cell_with_flag(0), 9u);
    EXPECT_EQ(gen::calc_raw_seq_cell_with_flag(256), 9u);
    EXPECT_EQ(gen::calc_raw_seq_cell_with_flag(257), 10u);

    EXPECT_EQ(gen::calc_seq_cell_with_flag(0), 9u);
    EXPECT_EQ(gen::calc_seq_cell_with_flag(256), 9u);
    EXPECT_EQ(gen::calc_seq_cell_with_flag(257), 18u);
    EXPECT_EQ(gen::calc_seq_cell_with_flag(131072), 18u);
    EXPECT_EQ(gen::calc_seq_cell_with_flag(131073), 27u);

    EXPECT_EQ(gen::calc_seq_cell_no_flag(), 9u);
    EXPECT_EQ(gen::calc_seq_cell_no_flag(10), 18u);
}

TEST(LuleaConfigTest, ComputesEdgeCases)
{
    // Edge case: 0 blocks in L2/L3 and small L2 subtable (e.g., 20-4-8)
    gen::LuleaArchDefines compact_defines;
    compact_defines.l1_ip_size = 20;
    compact_defines.l2_ip_size = 4;
    compact_defines.l3_ip_size = 8;

    const auto empty_params = gen::compute_lulea_parameters(compact_defines, gen::LuleaTableStatistics{});
    EXPECT_EQ(empty_params.l1.ch_sum_size, size_t{1} << 14);
    EXPECT_EQ(empty_params.l1.word_bus, 14u);
    EXPECT_EQ(empty_params.l1.seq_cell, 9u);
    EXPECT_EQ(empty_params.l2.chunks_per_block, 1u);
    EXPECT_EQ(empty_params.l2.ch_sum_size, 0u);
    EXPECT_EQ(empty_params.l2.word_bus, 0u);
    EXPECT_EQ(empty_params.l3.ch_sum_size, 0u);
    EXPECT_EQ(empty_params.l3.word_bus, 0u);
}
