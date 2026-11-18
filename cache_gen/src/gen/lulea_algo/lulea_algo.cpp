#include "lulea_algo.hpp"

#include "cache_set.hpp"
#include "gen.hpp"
#include "io/reader.hpp"
#include "lulea_tree.hpp"

#include <stdexcept>
#include <utility>

namespace gen
{
    namespace
    {
        class WriterState
        {
        private:
            size_t chunk_sum = 0;

        public:
            WriterState(size_t chunk_sum = 0) : chunk_sum(chunk_sum) {};
        };

        struct CacheLevelConfig
        {
            size_t ip_size = 0;
            size_t pc_cell = 0;
            size_t ch_sum_cell = 0;
            size_t seq_cell = 0;
            size_t chunks_per_block = 0;
        };

        void write_level_caches(const LuleaRoute &tree, LevelCaches &lcaches, const CacheLevelConfig &config)
        {
            size_t last = 0;
            for (const LuleaCell &cell : tree)
            {
                (void)cell;
            }
            (void)lcaches;
        }

        void generate_l1(const LuleaTree &tree, CacheSet &caches)
        {
            // make config
            const CacheLevelConfig l1_conf{
                .ip_size = L1_IP_SIZE,
                .pc_cell = L1_PC_CELL,
                .ch_sum_cell = L1_CH_SUM_CELL,
                .seq_cell = L1_DEFAULT_SEQ_CELL,
                .chunks_per_block = L1_CHUNKS_PER_BLOCK,
            };
            write_level_caches(tree.l1(), caches.l1(), l1_conf);
        }

        void generate_l2(const LuleaTree &tree, CacheSet &caches)
        {
            // make config
            const CacheLevelConfig l2_conf{
                .ip_size = L2_IP_SIZE,
                .pc_cell = L2_PC_CELL,
                .ch_sum_cell = L2_CH_SUM_CELL,
                .seq_cell = L2_DEFAULT_SEQ_CELL,
                .chunks_per_block = L2_CHUNKS_PER_BLOCK,
            };
            write_level_caches(tree.l1(), caches.l1(), l2_conf);
        }

        void generate_l3(const LuleaTree &tree, CacheSet &caches)
        {
            // make config
            const CacheLevelConfig l3_conf{
                .ip_size = L3_IP_SIZE,
                .pc_cell = L3_PC_CELL,
                .ch_sum_cell = L3_CH_SUM_CELL,
                .seq_cell = L3_DEFAULT_SEQ_CELL,
                .chunks_per_block = L3_CHUNKS_PER_BLOCK,
            };
            write_level_caches(tree.l1(), caches.l1(), l3_conf);
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
