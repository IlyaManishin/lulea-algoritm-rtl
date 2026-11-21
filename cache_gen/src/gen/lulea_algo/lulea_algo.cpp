#include "lulea_algo.hpp"

#include "errors.hpp"
#include "gen.hpp"
#include "io/bit_file_writer.hpp"
#include "io/reader.hpp"
#include "lulea_config.hpp"
#include "lulea_data/cache_set.hpp"
#include "lulea_data/route.hpp"
#include "lulea_tree.hpp"

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gen
{
    namespace
    {
        struct CacheLevelConfig
        {
            std::string_view level_name;

            size_t ip_size = 0;
            size_t pc_cell = 0;
            size_t ch_sum_cell = 0;
            size_t seq_cell = 0;
            size_t chunks_per_block = 0;
            size_t chunk_size = 0;
        };

        class LevelCellsView
        {
        public:
            explicit LevelCellsView(const LuleaRoute &single_block) noexcept
                : blocks_(&single_block, 1) {}

            explicit LevelCellsView(const std::vector<LuleaRoute> &blocks) noexcept
                : blocks_(blocks) {}

            class Iterator
            {
            public:
                Iterator(std::span<const LuleaRoute> blocks, size_t block_idx, size_t cell_idx) noexcept
                    : blocks_(blocks), block_idx_(block_idx), cell_idx_(cell_idx) {}

                const LuleaCell &operator*() const noexcept
                {
                    return blocks_[block_idx_][cell_idx_];
                }

                Iterator &operator++() noexcept
                {
                    cell_idx_++;
                    if (cell_idx_ == blocks_[block_idx_].size())
                    {
                        block_idx_++;
                        cell_idx_ = 0;
                    }
                    return *this;
                }

                bool operator!=(const Iterator &other) const noexcept
                {
                    return block_idx_ != other.block_idx_ || cell_idx_ != other.cell_idx_;
                }

            private:
                std::span<const LuleaRoute> blocks_;
                size_t block_idx_ = 0;
                size_t cell_idx_ = 0;
            };

            Iterator begin() const noexcept { return Iterator(blocks_, 0, 0); }
            Iterator end() const noexcept { return Iterator(blocks_, blocks_.size(), 0); }

            bool empty() const noexcept { return blocks_.empty(); }
            const LuleaCell &front() const noexcept { return blocks_[0][0]; }
            std::span<const LuleaRoute> blocks() const noexcept { return blocks_; }

        private:
            std::span<const LuleaRoute> blocks_;
        };

        void validate_level(std::span<const LuleaRoute> blocks, const CacheLevelConfig &conf)
        {
            if (conf.chunk_size == 0 || conf.chunks_per_block == 0)
            {
                throw std::runtime_error("Invalid chunk config for " + std::string(conf.level_name));
            }

            const size_t expected_block_size = conf.chunks_per_block * conf.chunk_size;
            size_t expected_bit_width = 0;

            for (const LuleaRoute &block : blocks)
            {
                if (block.size() != expected_block_size)
                {
                    throw std::runtime_error("Invalid block size for " + std::string(conf.level_name));
                }

                const size_t bit_width = block[0].bit_width();
                if (bit_width > conf.seq_cell)
                {
                    throw std::runtime_error("Cell bit width exceeds seq cell size for " + std::string(conf.level_name));
                }

                if (expected_bit_width == 0)
                {
                    expected_bit_width = bit_width;
                }
                else if (bit_width != expected_bit_width)
                {
                    throw std::runtime_error("Inconsistent cell bit width for " + std::string(conf.level_name));
                }
            }
        }

        void write_chunk_sum(BitFileWriter &cache, size_t ch_sum, size_t cell_size, std::string_view level)
        {
            if (cell_size < sizeof(size_t) * 8)
            {
                const size_t max_val = (size_t{1} << cell_size) - 1;
                if (ch_sum > max_val)
                {
                    throw LuleaOverflow(
                        std::string(level) + " chunk sum " + std::to_string(ch_sum) +
                        " exceeds cell capacity " + std::to_string(max_val));
                }
            }
            cache.write_bits(ch_sum, cell_size);
        }

        void write_level_caches(const LevelCellsView &cells,
                                LevelCaches &lcaches,
                                const CacheLevelConfig &conf)
        {
            validate_level(cells.blocks(), conf);

            if (cells.empty())
            {
                return;
            }

            BitFileWriter &pop_wr = lcaches.pop;
            BitFileWriter &chunk_wr = lcaches.chunk;
            BitFileWriter &seq_wr = lcaches.seq;

            size_t chunk_offset = conf.chunk_size;
            size_t chunk_sum = 0;

            size_t last_cell_raw = cells.front().raw();
            seq_wr.write_bits(last_cell_raw, conf.seq_cell);

            for (const LuleaCell &cell : cells)
            {
                if (chunk_offset == conf.chunk_size)
                {
                    write_chunk_sum(chunk_wr, chunk_sum, conf.ch_sum_cell, conf.level_name);
                    chunk_offset = 0;
                }
                chunk_offset++;

                if (cell.raw() != last_cell_raw)
                {
                    pop_wr.write_bits(1, 1);
                    seq_wr.write_bits(cell.raw(), conf.seq_cell);

                    last_cell_raw = cell.raw();
                    chunk_sum++;
                }
                else
                {
                    pop_wr.write_bits(0, 1);
                }
            }
        }

        void generate_l1(const LuleaTree &tree, CacheSet &caches)
        {
            const CacheLevelConfig l1_conf{
                .level_name = "L1",
                .ip_size = L1_IP_SIZE,
                .pc_cell = L1_PC_CELL,
                .ch_sum_cell = L1_CH_SUM_CELL,
                .seq_cell = L1_DEFAULT_SEQ_CELL,
                .chunks_per_block = L1_CHUNKS_PER_BLOCK,
                .chunk_size = PC_CHUNK_SIZE,
            };
            write_level_caches(LevelCellsView(tree.l1()), caches.l1(), l1_conf);
        }

        void generate_l2(const LuleaTree &tree, CacheSet &caches)
        {
            const CacheLevelConfig l2_conf{
                .level_name = "L2",
                .ip_size = L2_IP_SIZE,
                .pc_cell = L2_PC_CELL,
                .ch_sum_cell = L2_CH_SUM_CELL,
                .seq_cell = L2_DEFAULT_SEQ_CELL,
                .chunks_per_block = L2_CHUNKS_PER_BLOCK,
                .chunk_size = PC_CHUNK_SIZE,
            };
            write_level_caches(LevelCellsView(tree.l2()), caches.l2(), l2_conf);
        }

        void generate_l3(const LuleaTree &tree, CacheSet &caches)
        {
            const CacheLevelConfig l3_conf{
                .level_name = "L3",
                .ip_size = L3_IP_SIZE,
                .pc_cell = L3_PC_CELL,
                .ch_sum_cell = L3_CH_SUM_CELL,
                .seq_cell = L3_DEFAULT_SEQ_CELL,
                .chunks_per_block = L3_CHUNKS_PER_BLOCK,
                .chunk_size = PC_CHUNK_SIZE,
            };
            write_level_caches(LevelCellsView(tree.l3()), caches.l3(), l3_conf);
        }
    } // namespace

    LuleaAlgo::LuleaAlgo(LuleaTree routing_tree, Config config)
        : config_(routing_tree.config()), app_config_(std::move(config)), tree_(std::move(routing_tree)), initialized_(true) {}

    LuleaAlgo::LuleaAlgo(const LuleaTreeConfig &tree_config)
        : config_(tree_config), tree_(tree_config) {}

    LuleaAlgo::LuleaAlgo() : LuleaAlgo(LuleaTreeConfig{}) {}

    void LuleaAlgo::init(LuleaTree routing_tree)
    {
        this->config_ = routing_tree.config();
        this->tree_ = std::move(routing_tree);
        this->initialized_ = true;
        this->caches_generated_ = false;
    }

    void LuleaAlgo::init(const MaskEntries &entries)
    {
        this->tree_.build(entries);
        this->config_ = this->tree_.config();
        this->initialized_ = true;
        this->caches_generated_ = false;
    }

    CacheStats LuleaAlgo::generate_caches(bool write_files)
    {
        if (!this->initialized_)
        {
            throw std::logic_error("LuleaAlgo is not initialized with a routing tree");
        }

        CacheSet caches(this->app_config_.dest_dir, write_files);

        generate_l1(this->tree_, caches);
        generate_l2(this->tree_, caches);
        generate_l3(this->tree_, caches);

        caches.close();
        this->caches_generated_ = true;
        return caches.collect_stats();
    }

} // namespace gen
