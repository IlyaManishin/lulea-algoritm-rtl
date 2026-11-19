#pragma once

#include "gen.hpp"
#include "io/bit_file_writer.hpp"

#include <array>
#include <cstddef>
#include <filesystem>

namespace gen
{

    struct LevelCaches
    {
        BitFileWriter pop;
        BitFileWriter chunk;
        BitFileWriter seq;
    };

    using LevelWriters = LevelCaches;

    class CacheSet
    {
    public:
        explicit CacheSet(const std::filesystem::path &dest_dir = "", bool write_files = true);
        explicit CacheSet(bool write_files);

        bool write_files() const noexcept { return this->write_files_; }

        LevelCaches &level(size_t level_idx);

        LevelCaches &l1() noexcept { return this->levels_[0]; }
        LevelCaches &l2() noexcept { return this->levels_[1]; }
        LevelCaches &l3() noexcept { return this->levels_[2]; }

        BitFileWriter &l1_pop() noexcept { return this->levels_[0].pop; }
        BitFileWriter &l1_chunk() noexcept { return this->levels_[0].chunk; }
        BitFileWriter &l1_seq() noexcept { return this->levels_[0].seq; }

        BitFileWriter &l2_pop() noexcept { return this->levels_[1].pop; }
        BitFileWriter &l2_chunk() noexcept { return this->levels_[1].chunk; }
        BitFileWriter &l2_seq() noexcept { return this->levels_[1].seq; }

        BitFileWriter &l3_pop() noexcept { return this->levels_[2].pop; }
        BitFileWriter &l3_chunk() noexcept { return this->levels_[2].chunk; }
        BitFileWriter &l3_seq() noexcept { return this->levels_[2].seq; }

        CacheStats collect_stats() const;

        void flush();
        void close();

    private:
        bool write_files_ = true;
        std::array<LevelCaches, 3> levels_;
    };

} // namespace gen
