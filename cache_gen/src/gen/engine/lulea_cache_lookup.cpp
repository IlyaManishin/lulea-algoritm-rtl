#include "engine/lulea_cache_lookup.hpp"

#include "config.hpp"
#include "lulea_config.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace engine
{
    namespace
    {
        using seq_cell_t = uint32_t;
        using chunk_t = uint64_t;
        using chunk_sum_t = uint32_t;

        constexpr seq_cell_t PORT_MASK = (1U << PORT_SIZE) - 1;
        constexpr size_t CHUNK_BIT_MASK = PC_CHUNK_SIZE - 1;

        static_assert(gen::L1_IP_SIZE == 18 && gen::L2_IP_SIZE == 6 && gen::L3_IP_SIZE == 8 && gen::PC_CHUNK_SIZE == 64);
        static_assert(std::max({L1_DEFAULT_SEQ_CELL, L2_DEFAULT_SEQ_CELL, L3_DEFAULT_SEQ_CELL}) <= std::numeric_limits<seq_cell_t>::digits);
        static_assert(PC_CHUNK_SIZE <= std::numeric_limits<chunk_t>::digits);
        static_assert(CH_SUM_CELL_BITS <= std::numeric_limits<chunk_sum_t>::digits);

        struct LevelMemory
        {
            std::vector<chunk_t> chunks;
            std::vector<chunk_sum_t> chunk_sums;
            std::vector<seq_cell_t> seq;
        };

        std::vector<uint8_t> read_binary_file(const std::filesystem::path &path)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file.is_open())
            {
                throw std::runtime_error("Failed to open cache file: " + path.string());
            }

            const auto file_size = file.tellg();
            if (file_size < 0)
            {
                throw std::runtime_error("Failed to determine cache file size: " + path.string());
            }

            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
            if (file_size > 0 && !file.read(reinterpret_cast<char *>(buffer.data()), file_size))
            {
                throw std::runtime_error("Failed to read cache file: " + path.string());
            }
            return buffer;
        }

        template <typename StorageCell>
        std::vector<StorageCell> load_cache(const std::filesystem::path &path, size_t packed_width_bits)
        {
            // Each file is a continuous LSB-first stream, padded to the last byte.
            // Each packed value is stored in one C++ cell, which may be wider.
            const std::vector<uint8_t> bytes = read_binary_file(path);
            std::vector<StorageCell> cells(bytes.size() * 8 / packed_width_bits);
            size_t pos = 0;
            for (StorageCell &cell : cells)
            {
                for (size_t bit = 0; bit < packed_width_bits; ++bit, ++pos)
                {
                    const StorageCell value = (bytes[pos / 8] >> (pos % 8)) & 1U;
                    cell |= value << bit;
                }
            }
            return cells;
        }

        LevelMemory load_level(const std::filesystem::path &cache_dir, std::string_view prefix,
                               size_t seq_width_bits)
        {
            const std::string name(prefix);
            return {
                .chunks = load_cache<chunk_t>(cache_dir / (name + "_pop.bin"), PC_CHUNK_SIZE),
                .chunk_sums = load_cache<chunk_sum_t>(cache_dir / (name + "_chunk.bin"), CH_SUM_CELL_BITS),
                .seq = load_cache<seq_cell_t>(cache_dir / (name + "_seq.bin"), seq_width_bits),
            };
        }

        seq_cell_t read_cell(const LevelMemory &level, size_t chunk_addr, size_t bit_pos)
        {
            // Keep bits [bit_pos:0], including bit 63 for a full chunk.
            const chunk_t mask = std::numeric_limits<chunk_t>::max() >> (63U - bit_pos);
            const auto offset = static_cast<size_t>(std::popcount(level.chunks.at(chunk_addr) & mask));
            const size_t seq_addr = level.chunk_sums.at(chunk_addr) + offset;
            return level.seq.at(seq_addr);
        }
    } // namespace

    struct LuleaCacheLookup::Impl
    {
        LevelMemory l1;
        LevelMemory l2;
        LevelMemory l3;
    };

    LuleaCacheLookup::LuleaCacheLookup(const std::filesystem::path &cache_dir)
    {
        const Impl impl_val = {
            .l1 = load_level(cache_dir, "l1", L1_DEFAULT_SEQ_CELL),
            .l2 = load_level(cache_dir, "l2", L2_DEFAULT_SEQ_CELL),
            .l3 = load_level(cache_dir, "l3", L3_DEFAULT_SEQ_CELL),
        };
        impl_ = std::make_shared<const Impl>(impl_val);
    }

    port_t LuleaCacheLookup::lookup(ipv4_t ip) const
    {
        // L1: chunk address = ip[31:20], position = ip[19:14].
        const seq_cell_t l1_cell = read_cell(impl_->l1, ip >> 20, (ip >> 14) & CHUNK_BIT_MASK);
        constexpr seq_cell_t l1_flag_mask = 1U << (L1_DEFAULT_SEQ_CELL - 1);
        if ((l1_cell & l1_flag_mask) == 0)
        {
            return static_cast<port_t>(l1_cell & PORT_MASK); // is it safe casting, not a masking
        }
        constexpr seq_cell_t l1_ptr_mask = (l1_flag_mask - 1);

        // L2: one chunk per block, position = ip[13:8].
        const size_t l2_block = l1_cell & l1_ptr_mask;
        const seq_cell_t l2_cell = read_cell(impl_->l2, l2_block, (ip >> 8) & CHUNK_BIT_MASK);
        constexpr seq_cell_t l2_flag_mask = 1U << (L2_DEFAULT_SEQ_CELL - 1);
        if ((l2_cell & l2_flag_mask) == 0)
        {
            return static_cast<port_t>(l2_cell & PORT_MASK);
        }
        constexpr seq_cell_t l2_ptr_mask = (l2_flag_mask - 1);

        // L3: concatenate the block pointer with ip[7:6], position = ip[5:0].
        const size_t l3_block = l2_cell & l2_ptr_mask;
        const size_t chunk_addr = (l3_block << 2) | ((ip >> 6) & 3U);
        const seq_cell_t l3_cell = read_cell(impl_->l3, chunk_addr, ip & CHUNK_BIT_MASK);
        return static_cast<port_t>(l3_cell & PORT_MASK);
    }

} // namespace engine
