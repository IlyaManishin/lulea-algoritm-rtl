#pragma once

#include "gen.hpp"
#include "lulea_tree.hpp"
#include <lulea_common/reader.hpp>

#include <filesystem>

namespace gen
{

    class LuleaAlgo
    {
    public:
        explicit LuleaAlgo(LuleaTree routing_tree);
        LuleaAlgo() = default;

        void init(LuleaTree routing_tree);
        void init(const MaskEntries &entries);
        CacheStats generate_caches(const std::filesystem::path &dest_dir = {});

        const LuleaTree &tree() const noexcept { return this->tree_; }
        bool caches_generated() const noexcept { return this->caches_generated_; }

    private:
        LuleaTree tree_;
        bool caches_generated_ = false;
    };

} // namespace gen
