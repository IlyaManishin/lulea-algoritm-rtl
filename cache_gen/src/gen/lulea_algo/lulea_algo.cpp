#include "lulea_algo.hpp"

#include "cache.hpp"
#include "cache_set.hpp"
#include "errors.hpp"
#include "gen.hpp"
#include "io/bit_file_writer.hpp"
#include "io/reader.hpp"
#include "lulea_config.hpp"
#include "lulea_tree.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gen
{
    namespace
    {
        struct WriterState
        {
            size_t chunk_sum = 0;
        };

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

        void write_level_block(const LuleaRoute &block,
                               LevelCaches &lcaches,
                               const CacheLevelConfig &conf,
                               WriterState &state)
        {
            if (conf.chunk_size == 0 ||
                block.empty() ||
                block.size() % conf.chunk_size != 0 ||
                block.size() / conf.chunk_size != conf.chunks_per_block)
            {
                throw std::runtime_error("Invalid block size for " + std::string(conf.level_name));
            }
            if (block[0].bit_width() > conf.seq_cell)
            {
                throw std::runtime_error("Cell bit width exceeds seq cell size for " + std::string(conf.level_name));
            }

            BitFileWriter &pop_wr = lcaches.pop;
            BitFileWriter &chunk_wr = lcaches.chunk;
            BitFileWriter &seq_wr = lcaches.seq;

            size_t block_offset = conf.chunk_size;
            size_t last_cell_raw = block[0].raw();
            size_t chunk_sum = state.chunk_sum;

            for (const LuleaCell &cell : block)
            {
                if (block_offset == conf.chunk_size)
                {
                    write_chunk_sum(chunk_wr, chunk_sum, conf.ch_sum_cell, conf.level_name);
                    block_offset = 0;
                }
                block_offset++;

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
            state.chunk_sum = chunk_sum;
        }

        void write_level_block(const LuleaRoute &block,
                               LevelCaches &lcaches,
                               const CacheLevelConfig &conf)
        {
            WriterState state;
            write_level_block(block, lcaches, conf, state);
        }

        void write_level_caches(const std::vector<LuleaRoute> &level,
                                LevelCaches &lcaches,
                                const CacheLevelConfig &conf)
        {
            WriterState state;
            for (const auto &block : level)
            {
                write_level_block(block, lcaches, conf, state);
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
            write_level_block(tree.l1(), caches.l1(), l1_conf);
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
            write_level_caches(tree.l2(), caches.l2(), l2_conf);
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
            write_level_caches(tree.l3(), caches.l3(), l3_conf);
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
