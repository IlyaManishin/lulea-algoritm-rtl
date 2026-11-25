#pragma once

#include <lulea_common/config.hpp>
#include <lulea_common/reader.hpp>

#include <filesystem>
#include <memory>

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

    class ILookupEngineFactory
    {
    public:
        ILookupEngineFactory() = default;
        virtual ~ILookupEngineFactory() = default;
        ILookupEngineFactory(const ILookupEngineFactory &) = delete;
        ILookupEngineFactory &operator=(const ILookupEngineFactory &) = delete;
        ILookupEngineFactory(ILookupEngineFactory &&) = delete;
        ILookupEngineFactory &operator=(ILookupEngineFactory &&) = delete;

        virtual std::unique_ptr<ILookupEngine> get_lookup_engine(
            const std::filesystem::path &routes_path, const gen::MaskEntries &entries) = 0;
    };

} // namespace engine
