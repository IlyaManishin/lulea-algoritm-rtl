#pragma once

#include "gen.hpp"
#include "lulea_config.hpp"
#include "lulea_data/cache_set.hpp"
#include "lulea_tree.hpp"
#include <lulea_common/config.hpp>
#include <lulea_common/reader.hpp>

namespace gen
{

    class LuleaAlgo
    {
    public:
        explicit LuleaAlgo(LuleaTree routing_tree, Config config = {});
        explicit LuleaAlgo(const LuleaTreeConfig &tree_config);
        LuleaAlgo();

        void init(LuleaTree routing_tree);
        void init(const MaskEntries &entries);
        CacheStats generate_caches(bool write_files = true);

        const LuleaTreeConfig &config() const noexcept { return this->config_; }
        const Config &app_config() const noexcept { return this->app_config_; }
        const LuleaTree &tree() const noexcept { return this->tree_; }
        bool is_initialized() const noexcept { return this->initialized_; }
        bool caches_generated() const noexcept { return this->caches_generated_; }

    private:
        LuleaTreeConfig config_;
        Config app_config_;
        LuleaTree tree_;
        bool initialized_ = false;
        bool caches_generated_ = false;
    };

} // namespace gen
