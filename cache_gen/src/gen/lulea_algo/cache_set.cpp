#include "cache_set.hpp"

#include "gen.hpp"
#include "io/bit_file_writer.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>

namespace gen
{

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

    const LevelCaches &CacheSet::level(size_t level_idx) const
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
        return CacheStats{
            .cache_sizes = {
                .l1_pop = static_cast<uint32_t>((this->levels_[0].pop.total_bits_written() + 7U) / 8U),
                .l1_chunk = static_cast<uint32_t>((this->levels_[0].chunk.total_bits_written() + 7U) / 8U),
                .l1_seq = static_cast<uint32_t>((this->levels_[0].seq.total_bits_written() + 7U) / 8U),
                .l2_pop = static_cast<uint32_t>((this->levels_[1].pop.total_bits_written() + 7U) / 8U),
                .l2_chunk = static_cast<uint32_t>((this->levels_[1].chunk.total_bits_written() + 7U) / 8U),
                .l2_seq = static_cast<uint32_t>((this->levels_[1].seq.total_bits_written() + 7U) / 8U),
                .l3_pop = static_cast<uint32_t>((this->levels_[2].pop.total_bits_written() + 7U) / 8U),
                .l3_chunk = static_cast<uint32_t>((this->levels_[2].chunk.total_bits_written() + 7U) / 8U),
                .l3_seq = static_cast<uint32_t>((this->levels_[2].seq.total_bits_written() + 7U) / 8U),
            }
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
