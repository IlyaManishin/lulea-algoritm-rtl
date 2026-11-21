#include "cache_set.hpp"

#include "gen.hpp"
#include "io/bit_file_writer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>

namespace gen
{

    namespace
    {
        uint32_t cache_size_bytes(const BitFileWriter &writer, bool aligned, bool bitmap)
        {
            const size_t bits = writer.total_bits_written();
            if (!aligned)
            {
                return static_cast<uint32_t>((bits + 7) / 8);
            }

            // RAMB36: 512 x 64 usable bits. RAMB18: 1024 x 18 or 2048 x 9.
            const size_t usable_bits = bitmap ? 512 * 64 : 1024 * 18;
            const size_t block_bytes = bitmap ? 4608 : 2304;
            return static_cast<uint32_t>(((bits + usable_bits - 1) / usable_bits) * block_bytes);
        }

        CacheByteSizes collect_sizes(const std::array<LevelCaches, 3> &levels, bool aligned)
        {
            return {
                .l1_pop = cache_size_bytes(levels[0].pop, aligned, true),
                .l1_chunk = cache_size_bytes(levels[0].chunk, aligned, false),
                .l1_seq = cache_size_bytes(levels[0].seq, aligned, false),
                .l2_pop = cache_size_bytes(levels[1].pop, aligned, true),
                .l2_chunk = cache_size_bytes(levels[1].chunk, aligned, false),
                .l2_seq = cache_size_bytes(levels[1].seq, aligned, false),
                .l3_pop = cache_size_bytes(levels[2].pop, aligned, true),
                .l3_chunk = cache_size_bytes(levels[2].chunk, aligned, false),
                .l3_seq = cache_size_bytes(levels[2].seq, aligned, false),
            };
        }
    } // namespace

    CacheSet::CacheSet(const std::filesystem::path &dest_dir, bool write_files)
        : write_files_(write_files)
    {
        if (this->write_files_)
        {
            if (!dest_dir.empty())
            {
                std::filesystem::create_directories(dest_dir);
            }
            this->levels_[0] = LevelCaches{
                BitFileWriter(dest_dir / "l1_pop.bin"),
                BitFileWriter(dest_dir / "l1_chunk.bin"),
                BitFileWriter(dest_dir / "l1_seq.bin")
            };
            this->levels_[1] = LevelCaches{
                BitFileWriter(dest_dir / "l2_pop.bin"),
                BitFileWriter(dest_dir / "l2_chunk.bin"),
                BitFileWriter(dest_dir / "l2_seq.bin")
            };
            this->levels_[2] = LevelCaches{
                BitFileWriter(dest_dir / "l3_pop.bin"),
                BitFileWriter(dest_dir / "l3_chunk.bin"),
                BitFileWriter(dest_dir / "l3_seq.bin")
            };
        }
    }

    CacheSet::CacheSet(bool write_files)
        : CacheSet("", write_files) {}

    LevelCaches &CacheSet::level(size_t level_idx)
    {
        if (level_idx == 1)
        {
            return this->levels_[0];
        }
        if (level_idx == 2)
        {
            return this->levels_[1];
        }
        if (level_idx == 3)
        {
            return this->levels_[2];
        }
        throw std::out_of_range("Invalid cache level index");
    }

    CacheStats CacheSet::collect_stats() const
    {
        return {
            .cache_sizes = collect_sizes(this->levels_, false),
            .cache_sizes_aligned = collect_sizes(this->levels_, true),
        };
    }

    void CacheSet::flush()
    {
        for (auto &lvl : this->levels_)
        {
            lvl.pop.flush();
            lvl.chunk.flush();
            lvl.seq.flush();
        }
    }

    void CacheSet::close()
    {
        for (auto &lvl : this->levels_)
        {
            lvl.pop.close();
            lvl.chunk.close();
            lvl.seq.close();
        }
    }

} // namespace gen
