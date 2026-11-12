#pragma once

#include "config.hpp"
#include "core/lulea_tree.hpp"
#include "gen.hpp"
#include "io/reader.hpp"
#include "lulea_config.hpp"

namespace gen
{

    class LuleaAlgo
    {
    public:
        explicit LuleaAlgo(const LuleaTreeConfig &tree_config);
        LuleaAlgo();

        void init(const MaskEntries &entries);
        CacheStats generate_caches();

        const LuleaTreeConfig &config() const noexcept { return this->config_; }
        const LuleaTree &tree() const noexcept { return this->tree_; }
        bool is_initialized() const noexcept { return this->initialized_; }
        bool caches_generated() const noexcept { return this->caches_generated_; }

    private:
        LuleaTreeConfig config_;
        LuleaTree tree_;
        bool initialized_ = false;
        bool caches_generated_ = false;
    };

} // namespace gen
