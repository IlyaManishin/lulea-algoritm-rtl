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
        void generate_l1(const LuleaTree &tree, CacheSet &caches)
        {
            (void)tree;
            (void)caches;
        }

        void generate_l2(const LuleaTree &tree, CacheSet &caches)
        {
            (void)tree;
            (void)caches;
        }

        void generate_l3(const LuleaTree &tree, CacheSet &caches)
        {
            (void)tree;
            (void)caches;
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
