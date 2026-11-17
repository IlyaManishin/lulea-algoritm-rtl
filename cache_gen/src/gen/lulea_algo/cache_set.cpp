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
            this->l1_pop_ = BitFileWriter(dest_dir / "l1_pop.bin");
            this->l1_chunk_ = BitFileWriter(dest_dir / "l1_chunk.bin");
            this->l1_seq_ = BitFileWriter(dest_dir / "l1_seq.bin");
            this->l2_pop_ = BitFileWriter(dest_dir / "l2_pop.bin");
            this->l2_chunk_ = BitFileWriter(dest_dir / "l2_chunk.bin");
            this->l2_seq_ = BitFileWriter(dest_dir / "l2_seq.bin");
            this->l3_pop_ = BitFileWriter(dest_dir / "l3_pop.bin");
            this->l3_chunk_ = BitFileWriter(dest_dir / "l3_chunk.bin");
            this->l3_seq_ = BitFileWriter(dest_dir / "l3_seq.bin");
        }
    }

    CacheSet::CacheSet(bool write_files)
        : CacheSet("", write_files) {}

    LevelWriters CacheSet::level(size_t level_idx)
    {
        if (level_idx == 1)
        {
            return {this->l1_pop_, this->l1_chunk_, this->l1_seq_};
        }
        if (level_idx == 2)
        {
            return {this->l2_pop_, this->l2_chunk_, this->l2_seq_};
        }
        if (level_idx == 3)
        {
            return {this->l3_pop_, this->l3_chunk_, this->l3_seq_};
        }
        throw std::out_of_range("Invalid cache level index");
    }

    ConstLevelWriters CacheSet::level(size_t level_idx) const
    {
        if (level_idx == 1)
        {
            return {this->l1_pop_, this->l1_chunk_, this->l1_seq_};
        }
        if (level_idx == 2)
        {
            return {this->l2_pop_, this->l2_chunk_, this->l2_seq_};
        }
        if (level_idx == 3)
        {
            return {this->l3_pop_, this->l3_chunk_, this->l3_seq_};
        }
        throw std::out_of_range("Invalid cache level index");
    }

    CacheStats CacheSet::collect_stats() const
    {
        return CacheStats{
            .cache_sizes = {
                .l1_pop = static_cast<uint32_t>((this->l1_pop_.total_bits_written() + 7U) / 8U),
                .l1_chunk = static_cast<uint32_t>((this->l1_chunk_.total_bits_written() + 7U) / 8U),
                .l1_seq = static_cast<uint32_t>((this->l1_seq_.total_bits_written() + 7U) / 8U),
                .l2_pop = static_cast<uint32_t>((this->l2_pop_.total_bits_written() + 7U) / 8U),
                .l2_chunk = static_cast<uint32_t>((this->l2_chunk_.total_bits_written() + 7U) / 8U),
                .l2_seq = static_cast<uint32_t>((this->l2_seq_.total_bits_written() + 7U) / 8U),
                .l3_pop = static_cast<uint32_t>((this->l3_pop_.total_bits_written() + 7U) / 8U),
                .l3_chunk = static_cast<uint32_t>((this->l3_chunk_.total_bits_written() + 7U) / 8U),
                .l3_seq = static_cast<uint32_t>((this->l3_seq_.total_bits_written() + 7U) / 8U),
            }
        };
    }

    void CacheSet::flush()
    {
        this->l1_pop_.flush();
        this->l1_chunk_.flush();
        this->l1_seq_.flush();
        this->l2_pop_.flush();
        this->l2_chunk_.flush();
        this->l2_seq_.flush();
        this->l3_pop_.flush();
        this->l3_chunk_.flush();
        this->l3_seq_.flush();
    }

    void CacheSet::close()
    {
        this->l1_pop_.close();
        this->l1_chunk_.close();
        this->l1_seq_.close();
        this->l2_pop_.close();
        this->l2_chunk_.close();
        this->l2_seq_.close();
        this->l3_pop_.close();
        this->l3_chunk_.close();
        this->l3_seq_.close();
    }

} // namespace gen
