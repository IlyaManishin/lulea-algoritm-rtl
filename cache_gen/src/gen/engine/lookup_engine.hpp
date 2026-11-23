#pragma once

#include "config.hpp"

namespace engine
{
    using gen::ipv4_t;
    using gen::port_t;

    class ILookupEngine
    {
    public:
        ILookupEngine() = default;
        virtual ~ILookupEngine() = default;
        ILookupEngine(const ILookupEngine &) = default;
        ILookupEngine &operator=(const ILookupEngine &) = default;
        ILookupEngine(ILookupEngine &&) noexcept = default;
        ILookupEngine &operator=(ILookupEngine &&) noexcept = default;

        // Universal lookup interface: resolves IPv4 address to egress port
        virtual port_t lookup(ipv4_t ip) const = 0;
    };

    using LookupEngine = ILookupEngine; // delete this

    class ILookupEngineFactory
    {

    public:
        ILookupEngineFactory() = default;
        virtual std::unique_ptr<engine::ILookupEngine> get_lookup_engine(const fs::path &dump_dir) = delete;
    };



} // namespace engine
