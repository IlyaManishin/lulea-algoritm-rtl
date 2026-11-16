#include "core/lulea_algo.hpp"

#include "core/lulea_tree.hpp"
#include "gen.hpp"
#include "io/reader.hpp"

#include <stdexcept>
#include <utility>

namespace gen
{

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

    CacheStats LuleaAlgo::generate_caches()
    {
        if (!this->initialized_)
        {
            throw std::logic_error("LuleaAlgo is not initialized with a routing tree");
        }

        // Framework for generating L1, L2, and L3 caches (POP, CHUNK, SEQ).
        this->caches_generated_ = true;
        return {};
    }

} // namespace gen
