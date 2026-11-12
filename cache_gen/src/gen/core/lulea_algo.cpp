#include "core/lulea_algo.hpp"

#include "core/lulea_tree.hpp"
#include "gen.hpp"
#include "io/reader.hpp"

namespace gen
{

    LuleaAlgo::LuleaAlgo(const LuleaTreeConfig &tree_config)
        : config_(tree_config), tree_(tree_config)
    {
    }

    LuleaAlgo::LuleaAlgo() : LuleaAlgo(LuleaTreeConfig{}) {}

    void LuleaAlgo::init(const MaskEntries &entries)
    {
        this->tree_.build(entries);
        this->initialized_ = true;
        this->caches_generated_ = false;
    }

    CacheStats LuleaAlgo::generate_caches()
    {
        if (!this->initialized_)
        {
            return {};
        }

        // Framework for generating L1, L2, and L3 caches (POP, CHUNK, SEQ).
        this->caches_generated_ = true;
        return {};
    }

} // namespace gen
