#include "engine/lulea_cache_lookup.hpp"

#include "config.hpp"
#include "lulea_config.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace engine
{
    namespace
    {
        static_assert(gen::L1_IP_SIZE == 18 && gen::L2_IP_SIZE == 6 && gen::L3_IP_SIZE == 8 && gen::PC_CHUNK_SIZE == 64);

        struct CacheLevelBuffers
        {
            std::vector<uint8_t> pop;
            std::vector<uint8_t> chunk;
            std::vector<uint8_t> seq;
        };

        struct LevelMemory
        {
            std::vector<uint64_t> pop;
            std::vector<uint32_t> chunk;
            std::vector<uint32_t> seq;
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

        CacheLevelBuffers load_level_buffers(const std::filesystem::path &cache_dir, std::string_view prefix)
        {
            const std::string name(prefix);
            return {
                .pop = read_binary_file(cache_dir / (name + "_pop.bin")),
                .chunk = read_binary_file(cache_dir / (name + "_chunk.bin")),
                .seq = read_binary_file(cache_dir / (name + "_seq.bin")),
            };
        }

        template <typename Word>
        std::vector<Word> unpack(const std::vector<uint8_t> &bytes, size_t width)
        {
            // Each file is a continuous LSB-first stream, padded to the last byte.
            std::vector<Word> words(bytes.size() * 8 / width);
            size_t pos = 0;
            for (Word &word : words)
            {
                for (size_t bit = 0; bit < width; ++bit, ++pos)
                {
                    const Word value = (bytes[pos / 8] >> (pos % 8)) & 1U;
                    word |= value << bit;
                }
            }
            return words;
        }

        LevelMemory unpack_level(const CacheLevelBuffers &buffers, size_t seq_width)
        {
            return {unpack<uint64_t>(buffers.pop, PC_CHUNK_SIZE),
                    unpack<uint32_t>(buffers.chunk, CH_SUM_CELL_BITS),
                    unpack<uint32_t>(buffers.seq, seq_width)};
        }

        uint32_t read_cell(const LevelMemory &level, size_t word_addr, size_t bit_pos)
        {
            // Keep bits [bit_pos:0], including bit 63 for a full word.
            const uint64_t mask = UINT64_MAX >> (63U - bit_pos);
            const auto offset = static_cast<uint32_t>(std::popcount(level.pop.at(word_addr) & mask));
            const size_t seq_addr = level.chunk.at(word_addr) + offset;
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
        Impl impl_val = {
            .l1 = unpack_level(load_level_buffers(cache_dir, "l1"), L1_DEFAULT_SEQ_CELL),
            .l2 = unpack_level(load_level_buffers(cache_dir, "l2"), L2_DEFAULT_SEQ_CELL),
            .l3 = unpack_level(load_level_buffers(cache_dir, "l3"), L3_DEFAULT_SEQ_CELL),
        };
        impl_ = std::make_shared<const Impl>(impl_val);
    }

    port_t LuleaCacheLookup::lookup(ipv4_t ip) const
    {
        // L1: word = ip[31:20], position = ip[19:14].
        const uint32_t l1_cell = read_cell(impl_->l1, ip >> 20, (ip >> 14) & 63U);
        constexpr uint32_t l1_flag = 1U << (L1_DEFAULT_SEQ_CELL - 1);
        if ((l1_cell & l1_flag) == 0)
        {
            return static_cast<port_t>(l1_cell);
        }

        // L2: one word per block, position = ip[13:8].
        const uint32_t l2_block = l1_cell & (l1_flag - 1);
        const uint32_t l2_cell = read_cell(impl_->l2, l2_block, (ip >> 8) & 63U);
        constexpr uint32_t l2_flag = 1U << (L2_DEFAULT_SEQ_CELL - 1);
        if ((l2_cell & l2_flag) == 0)
        {
            return static_cast<port_t>(l2_cell);
        }

        // L3: concatenate the block pointer with ip[7:6], position = ip[5:0].
        const uint32_t l3_block = l2_cell & (l2_flag - 1);
        const size_t word_addr = (l3_block << 2) | ((ip >> 6) & 3U);
        return static_cast<port_t>(read_cell(impl_->l3, word_addr, ip & 63U) & 0xffU);
    }

} // namespace engine
