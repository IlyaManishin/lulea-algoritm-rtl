#pragma once

#include "lulea_config.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace gen
{

    // =========================================================================
    //  RTL / BRAM BIT-WIDTH AND POPCOUNT CONSTANTS
    // =========================================================================

    inline constexpr size_t PC_BIT_POS_BITS = static_cast<size_t>(std::bit_width(PC_CHUNK_SIZE - 1)); // 6 bits [5:0]
    inline constexpr size_t PC_OFFSET_BITS = static_cast<size_t>(std::bit_width(PC_CHUNK_SIZE));      // 7 bits [6:0]

    // =========================================================================
    //  PARAMETER FORMULAS
    // =========================================================================

    constexpr size_t clog2(size_t value) noexcept
    {
        return (value <= 1) ? 0 : static_cast<size_t>(std::bit_width(value - 1));
    }

    constexpr size_t align_to_bram_cell(
        size_t bits,
        size_t align_bits = BRAM_CELL_ALIGN_BITS) noexcept
    {
        if (bits == 0 || align_bits == 0)
        {
            return bits;
        }
        return ((bits + align_bits - 1) / align_bits) * align_bits;
    }

    constexpr size_t calc_chunks_per_block(
        size_t ip_size_bits,
        size_t chunk_size_bits = PC_CHUNK_SIZE) noexcept
    {
        if (chunk_size_bits == 0)
        {
            return 0;
        }
        const size_t block_entries = size_t{1} << ip_size_bits;
        return (block_entries + chunk_size_bits - 1) / chunk_size_bits;
    }

    constexpr size_t calc_ch_sum_size(
        size_t ip_size_bits,
        size_t pc_count,
        size_t chunk_size_bits = PC_CHUNK_SIZE) noexcept
    {
        return calc_chunks_per_block(ip_size_bits, chunk_size_bits) * pc_count;
    }

    constexpr size_t calc_raw_seq_cell_with_flag(
        size_t next_pc_count,
        size_t port_size_bits = PORT_SIZE) noexcept
    {
        return SEQ_FLAG_BITS + std::max(clog2(next_pc_count), port_size_bits);
    }

    constexpr size_t calc_seq_cell_with_flag(
        size_t next_pc_count,
        size_t port_size_bits = PORT_SIZE,
        size_t align_bits = BRAM_CELL_ALIGN_BITS) noexcept
    {
        return align_to_bram_cell(
            calc_raw_seq_cell_with_flag(next_pc_count, port_size_bits),
            align_bits);
    }

    constexpr size_t calc_seq_cell_no_flag(
        size_t port_size_bits = PORT_SIZE,
        size_t align_bits = BRAM_CELL_ALIGN_BITS) noexcept
    {
        return align_to_bram_cell(port_size_bits, align_bits);
    }

    // Derived static parameters for 18_6_8
    inline constexpr size_t L1_CHUNKS_PER_BLOCK = calc_chunks_per_block(L1_IP_SIZE); // 4096
    inline constexpr size_t L1_CH_SUM_SIZE = calc_ch_sum_size(L1_IP_SIZE, L1_PC_COUNT); // 4096
    inline constexpr size_t L1_WORD_BUS = clog2(L1_CH_SUM_SIZE); // 12 bits

    inline constexpr size_t L2_CHUNKS_PER_BLOCK = calc_chunks_per_block(L2_IP_SIZE); // 1
    inline constexpr size_t L3_CHUNKS_PER_BLOCK = calc_chunks_per_block(L3_IP_SIZE); // 4
    inline constexpr size_t L3_WORD_OFFSET_BITS = clog2(L3_CHUNKS_PER_BLOCK); // 2 bits
    inline constexpr size_t L3_SEQ_CELL = calc_seq_cell_no_flag(); // 9 bits

    // =========================================================================
    //  CONFIGURATION, STATISTICS, AND CALCULATED PARAMETERS
    // =========================================================================

    struct LuleaArchDefines
    {
        // Global defines
        size_t port_size = PORT_SIZE;
        size_t pc_chunk_size = PC_CHUNK_SIZE;
        size_t bram_align_bits = BRAM_CELL_ALIGN_BITS;

        // L1 defines
        size_t l1_ip_size = L1_IP_SIZE;
        size_t l1_pc_count = L1_PC_COUNT;
        size_t l1_ch_sum_cell = L1_CH_SUM_CELL;

        // L2 defines
        size_t l2_ip_size = L2_IP_SIZE;
        size_t l2_ch_sum_cell = L2_CH_SUM_CELL;

        // L3 defines
        size_t l3_ip_size = L3_IP_SIZE;
        size_t l3_ch_sum_cell = L3_CH_SUM_CELL;
    };

    struct LuleaTableStatistics
    {
        // L1 statistics
        size_t l1_seq_size = 0;

        // L2 statistics
        size_t l2_pc_count = 0;
        size_t l2_seq_size = 0;

        // L3 statistics
        size_t l3_pc_count = 0;
        size_t l3_seq_size = 0;
    };

    struct LuleaLevelParameters
    {
        // Geometry & block counts
        size_t chunks_per_block = 0;
        size_t ch_sum_size = 0;

        // BRAM cell widths (bits)
        size_t pc_cell = 0;
        size_t ch_sum_cell = 0;
        size_t seq_cell = 0;

        // Address bus widths (bits)
        size_t pc_bus = 0;
        size_t word_bus = 0;
        size_t seq_bus = 0;
    };

    struct LuleaParameters
    {
        LuleaLevelParameters l1;
        LuleaLevelParameters l2;
        LuleaLevelParameters l3;
    };

    constexpr LuleaParameters compute_lulea_parameters(
        const LuleaArchDefines &defines,
        const LuleaTableStatistics &stats) noexcept
    {
        const size_t l1_chunks = calc_chunks_per_block(defines.l1_ip_size, defines.pc_chunk_size);
        const size_t l2_chunks = calc_chunks_per_block(defines.l2_ip_size, defines.pc_chunk_size);
        const size_t l3_chunks = calc_chunks_per_block(defines.l3_ip_size, defines.pc_chunk_size);

        const size_t l1_ch_sum_size = l1_chunks * defines.l1_pc_count;
        const size_t l2_ch_sum_size = l2_chunks * stats.l2_pc_count;
        const size_t l3_ch_sum_size = l3_chunks * stats.l3_pc_count;

        return LuleaParameters{
            .l1 = LuleaLevelParameters{
                .chunks_per_block = l1_chunks,
                .ch_sum_size = l1_ch_sum_size,
                .pc_cell = defines.pc_chunk_size,
                .ch_sum_cell = defines.l1_ch_sum_cell,
                .seq_cell = calc_seq_cell_with_flag(
                    stats.l2_pc_count, defines.port_size, defines.bram_align_bits),
                .pc_bus = clog2(defines.l1_pc_count),
                .word_bus = clog2(l1_ch_sum_size),
                .seq_bus = clog2(stats.l1_seq_size),
            },
            .l2 = LuleaLevelParameters{
                .chunks_per_block = l2_chunks,
                .ch_sum_size = l2_ch_sum_size,
                .pc_cell = defines.pc_chunk_size,
                .ch_sum_cell = defines.l2_ch_sum_cell,
                .seq_cell = calc_seq_cell_with_flag(
                    stats.l3_pc_count, defines.port_size, defines.bram_align_bits),
                .pc_bus = clog2(stats.l2_pc_count),
                .word_bus = clog2(l2_ch_sum_size),
                .seq_bus = clog2(stats.l2_seq_size),
            },
            .l3 = LuleaLevelParameters{
                .chunks_per_block = l3_chunks,
                .ch_sum_size = l3_ch_sum_size,
                .pc_cell = defines.pc_chunk_size,
                .ch_sum_cell = defines.l3_ch_sum_cell,
                .seq_cell = calc_seq_cell_no_flag(defines.port_size, defines.bram_align_bits),
                .pc_bus = clog2(stats.l3_pc_count),
                .word_bus = clog2(l3_ch_sum_size),
                .seq_bus = clog2(stats.l3_seq_size),
            },
        };
    }

} // namespace gen
