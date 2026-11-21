#pragma once

#include "config.hpp"
#include "engine/lulea_tree_lookup.hpp"

#include <filesystem>
#include <memory>

namespace engine
{
    class LuleaCacheLookup : public ILookupEngine
    {
    public:
        explicit LuleaCacheLookup(const std::filesystem::path &cache_dir);
        port_t lookup(ipv4_t ip) const override;

    private:
        struct Impl;
        std::shared_ptr<const Impl> impl_;
    };

} // namespace engine
