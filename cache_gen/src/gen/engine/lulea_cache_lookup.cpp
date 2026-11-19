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
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine
{
    namespace
    {
        struct LevelLayout
        {
            size_t ip_shift;
            uint32_t ip_mask;
            size_t chunks_per_block_shift;
            size_t ch_sum_cell;
            size_t seq_cell;
            size_t flag_shift;
            uint32_t payload_mask;
        };

        struct DecodedCell
        {
            bool is_ptr;
            uint32_t payload;
        };

        constexpr uint64_t bit_mask(size_t width) noexcept
        {
            return (width >= 64) ? ~uint64_t{0} : ((uint64_t{1} << width) - 1);
        }

        constexpr LevelLayout make_layout(size_t ip_shift,
                                          size_t ip_bits,
                                          size_t chunks_per_block,
                                          size_t ch_sum_cell,
                                          size_t seq_cell) noexcept
        {
            return {
                .ip_shift = ip_shift,
                .ip_mask = static_cast<uint32_t>(bit_mask(ip_bits)),
                .chunks_per_block_shift = static_cast<size_t>(std::countr_zero(chunks_per_block)),
                .ch_sum_cell = ch_sum_cell,
                .seq_cell = seq_cell,
                .flag_shift = seq_cell - 1,
                .payload_mask = static_cast<uint32_t>(bit_mask(seq_cell - 1)),
            };
        }

        constexpr size_t CHUNK_SIZE = static_cast<size_t>(PC_CHUNK_SIZE);
        constexpr size_t CHUNK_SHIFT = static_cast<size_t>(std::countr_zero(CHUNK_SIZE));
        constexpr size_t CHUNK_MASK = CHUNK_SIZE - 1;

        constexpr LevelLayout L1_LAYOUT = make_layout(
            L2_IP_SIZE + L3_IP_SIZE, L1_IP_SIZE, L1_CHUNKS_PER_BLOCK, L1_CH_SUM_CELL, L1_DEFAULT_SEQ_CELL);
        constexpr LevelLayout L2_LAYOUT = make_layout(
            L3_IP_SIZE, L2_IP_SIZE, L2_CHUNKS_PER_BLOCK, L2_CH_SUM_CELL, L2_DEFAULT_SEQ_CELL);
        constexpr LevelLayout L3_LAYOUT = make_layout(
            0, L3_IP_SIZE, L3_CHUNKS_PER_BLOCK, L3_CH_SUM_CELL, L3_DEFAULT_SEQ_CELL);

        constexpr size_t extract_slice(ipv4_t ip, const LevelLayout &layout) noexcept
        {
            return static_cast<size_t>((ip >> layout.ip_shift) & layout.ip_mask);
        }

        constexpr uint64_t prefix_mask(size_t bit_pos) noexcept
        {
            return (~uint64_t{0}) >> (63 - bit_pos);
        }

        constexpr DecodedCell decode_branch_cell(uint32_t raw_cell, const LevelLayout &layout) noexcept
        {
            return {
                .is_ptr = ((raw_cell >> layout.flag_shift) & uint32_t{1}) != 0,
                .payload = raw_cell & layout.payload_mask,
            };
        }

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

        CacheLevelBuffers load_level_buffers(const std::filesystem::path &cache_dir, std::string_view prefix)
        {
            const std::string name(prefix);
            return {
                .pop = read_binary_file(cache_dir / (name + "_pop.bin")),
                .chunk = read_binary_file(cache_dir / (name + "_chunk.bin")),
                .seq = read_binary_file(cache_dir / (name + "_seq.bin")),
            };
        }

        uint64_t read_bits(const std::vector<uint8_t> &data, size_t bit_offset, size_t num_bits)
        {
            if (num_bits == 0)
            {
                return 0;
            }
            if (num_bits > 64)
            {
                throw std::invalid_argument("Cannot read more than 64 bits");
            }

            uint64_t result = 0;
            size_t byte_idx = bit_offset >> 3;
            size_t bit_idx = bit_offset & 7;
            size_t bits_read = 0;

            while (bits_read < num_bits)
            {
                if (byte_idx >= data.size())
                {
                    throw std::out_of_range("Attempt to read past end of cache buffer");
                }

                const size_t bits_to_read = std::min(num_bits - bits_read, 8 - bit_idx);
                const uint64_t chunk = (static_cast<uint64_t>(data[byte_idx]) >> bit_idx) & bit_mask(bits_to_read);
                result |= (chunk << bits_read);

                bits_read += bits_to_read;
                byte_idx++;
                bit_idx = 0;
            }
            return result;
        }

        uint32_t lookup_level_raw(const CacheLevelBuffers &buffers,
                                  const LevelLayout &layout,
                                  size_t block_idx,
                                  size_t entry_idx)
        {
            const size_t chunk_in_block = entry_idx >> CHUNK_SHIFT;
            const size_t word_addr = (block_idx << layout.chunks_per_block_shift) | chunk_in_block;
            const size_t bit_pos = entry_idx & CHUNK_MASK;

            const uint64_t pop_word = read_bits(buffers.pop, word_addr << CHUNK_SHIFT, CHUNK_SIZE);
            const size_t offset = static_cast<size_t>(std::popcount(pop_word & prefix_mask(bit_pos)));
            const size_t chunk_sum = static_cast<size_t>(
                read_bits(buffers.chunk, word_addr * layout.ch_sum_cell, layout.ch_sum_cell));
            const size_t seq_addr = chunk_sum + offset;

            return static_cast<uint32_t>(
                read_bits(buffers.seq, seq_addr * layout.seq_cell, layout.seq_cell));
        }
    } // namespace

    LuleaCacheLookup::LuleaCacheLookup(const std::filesystem::path &cache_dir)
        : l1_(load_level_buffers(cache_dir, "l1")),
          l2_(load_level_buffers(cache_dir, "l2")),
          l3_(load_level_buffers(cache_dir, "l3"))
    {
    }

    LuleaCacheLookup::LuleaCacheLookup(CacheLevelBuffers l1, CacheLevelBuffers l2, CacheLevelBuffers l3)
        : l1_(std::move(l1)), l2_(std::move(l2)), l3_(std::move(l3))
    {
    }

    port_t LuleaCacheLookup::lookup(ipv4_t ip) const
    {
        const size_t l1_idx = extract_slice(ip, L1_LAYOUT);
        const DecodedCell l1_cell = decode_branch_cell(
            lookup_level_raw(this->l1_, L1_LAYOUT, 0, l1_idx),
            L1_LAYOUT);
        if (!l1_cell.is_ptr)
        {
            return static_cast<port_t>(l1_cell.payload);
        }

        const size_t l2_idx = extract_slice(ip, L2_LAYOUT);
        const DecodedCell l2_cell = decode_branch_cell(
            lookup_level_raw(this->l2_, L2_LAYOUT, l1_cell.payload, l2_idx),
            L2_LAYOUT);
        if (!l2_cell.is_ptr)
        {
            return static_cast<port_t>(l2_cell.payload);
        }

        const size_t l3_idx = extract_slice(ip, L3_LAYOUT);
        const uint32_t l3_raw = lookup_level_raw(this->l3_, L3_LAYOUT, l2_cell.payload, l3_idx);
        return static_cast<port_t>(l3_raw & L3_LAYOUT.payload_mask);
    }

} // namespace engine