#pragma once

#include <lulea_common/config.hpp>
#include <lulea_verify/lookup_engine.hpp>

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
