#pragma once

#include "gen.hpp"
#include "io/bit_file_writer.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace gen
{

    struct LevelWriters
    {
        BitFileWriter &pop;
        BitFileWriter &chunk;
        BitFileWriter &seq;
    };

    struct ConstLevelWriters
    {
        const BitFileWriter &pop;
        const BitFileWriter &chunk;
        const BitFileWriter &seq;
    };

    class CacheSet
    {
    public:
        explicit CacheSet(const std::filesystem::path &dest_dir = "", bool write_files = true);
        explicit CacheSet(bool write_files);

        bool write_files() const noexcept { return this->write_files_; }

        BitFileWriter &l1_pop() noexcept { return this->l1_pop_; }
        BitFileWriter &l1_chunk() noexcept { return this->l1_chunk_; }
        BitFileWriter &l1_seq() noexcept { return this->l1_seq_; }

        BitFileWriter &l2_pop() noexcept { return this->l2_pop_; }
        BitFileWriter &l2_chunk() noexcept { return this->l2_chunk_; }
        BitFileWriter &l2_seq() noexcept { return this->l2_seq_; }

        BitFileWriter &l3_pop() noexcept { return this->l3_pop_; }
        BitFileWriter &l3_chunk() noexcept { return this->l3_chunk_; }
        BitFileWriter &l3_seq() noexcept { return this->l3_seq_; }

        const BitFileWriter &l1_pop() const noexcept { return this->l1_pop_; }
        const BitFileWriter &l1_chunk() const noexcept { return this->l1_chunk_; }
        const BitFileWriter &l1_seq() const noexcept { return this->l1_seq_; }

        const BitFileWriter &l2_pop() const noexcept { return this->l2_pop_; }
        const BitFileWriter &l2_chunk() const noexcept { return this->l2_chunk_; }
        const BitFileWriter &l2_seq() const noexcept { return this->l2_seq_; }

        const BitFileWriter &l3_pop() const noexcept { return this->l3_pop_; }
        const BitFileWriter &l3_chunk() const noexcept { return this->l3_chunk_; }
        const BitFileWriter &l3_seq() const noexcept { return this->l3_seq_; }

        LevelWriters level(size_t level_idx);
        ConstLevelWriters level(size_t level_idx) const;

        CacheStats collect_stats() const;

        void flush();
        void close();

    private:
        bool write_files_ = true;
        BitFileWriter l1_pop_;
        BitFileWriter l1_chunk_;
        BitFileWriter l1_seq_;
        BitFileWriter l2_pop_;
        BitFileWriter l2_chunk_;
        BitFileWriter l2_seq_;
        BitFileWriter l3_pop_;
        BitFileWriter l3_chunk_;
        BitFileWriter l3_seq_;
    };

} // namespace gen
