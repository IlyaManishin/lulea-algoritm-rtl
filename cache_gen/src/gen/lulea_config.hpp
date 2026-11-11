#pragma once

#include "gen/config.hpp"

#include <cstddef>
#include <limits>

namespace gen
{

    // =========================================================================
    //  GLOBAL DEFINES (docs/implementation_18_6_8.drawio -> configuration)
    // =========================================================================

    inline constexpr size_t IP_SIZE_BITS =
        sizeof(ipv4_t) * static_cast<size_t>(std::numeric_limits<unsigned char>::digits);
    inline constexpr size_t PORT_SIZE = 8;

    inline constexpr size_t PC_CHUNK_BYTES = 8;
    inline constexpr size_t PC_CHUNK_SIZE = PC_CHUNK_BYTES * 8; // 64 bits (chunk [63:0])

    inline constexpr size_t SEQ_FLAG_BITS = 1;
    inline constexpr size_t BRAM_CELL_ALIGN_BITS = 9;
    inline constexpr size_t CH_SUM_CELL_BITS = 2 * BRAM_CELL_ALIGN_BITS; // 18 bits

    // =========================================================================
    //  L1 CACHE (18 bits: ip[31:14])
    // =========================================================================

    inline constexpr size_t L1_IP_SIZE = 18;
    inline constexpr size_t L1_PC_COUNT = 1;

    inline constexpr size_t L1_PC_CELL = PC_CHUNK_SIZE;
    inline constexpr size_t L1_CH_SUM_CELL = CH_SUM_CELL_BITS;
    inline constexpr size_t L1_DEFAULT_SEQ_CELL = 2 * BRAM_CELL_ALIGN_BITS; // 18 bits

    // =========================================================================
    //  L2 CACHE (6 bits: ip[13:8])
    // =========================================================================

    inline constexpr size_t L2_IP_SIZE = 6;

    inline constexpr size_t L2_PC_CELL = PC_CHUNK_SIZE;
    inline constexpr size_t L2_CH_SUM_CELL = CH_SUM_CELL_BITS;
    inline constexpr size_t L2_DEFAULT_SEQ_CELL = BRAM_CELL_ALIGN_BITS; // 9 bits

    // =========================================================================
    //  L3 CACHE (8 bits: ip[7:0])
    // =========================================================================

    inline constexpr size_t L3_IP_SIZE = 8;

    inline constexpr size_t L3_PC_CELL = PC_CHUNK_SIZE;
    inline constexpr size_t L3_CH_SUM_CELL = CH_SUM_CELL_BITS;
    inline constexpr size_t L3_DEFAULT_SEQ_CELL = BRAM_CELL_ALIGN_BITS; // 9 bits

    static_assert(
        PORT_SIZE <= (sizeof(port_t) * static_cast<size_t>(std::numeric_limits<unsigned char>::digits)),
        "PORT_SIZE must fit within port_t");
    static_assert(
        (L1_IP_SIZE + L2_IP_SIZE + L3_IP_SIZE) == IP_SIZE_BITS,
        "Default L1/L2/L3 prefix split must sum to IP_SIZE_BITS");
    static_assert(
        (PC_CHUNK_SIZE > 0) && ((PC_CHUNK_SIZE & (PC_CHUNK_SIZE - 1)) == 0),
        "PC_CHUNK_SIZE must be a non-zero power of two");

} // namespace gen
