#pragma once

#include "config.hpp"
#include "core/lulea_tree.hpp"

namespace engine
{
    using namespace gen;

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

    using LookupEngine = ILookupEngine;

    class LuleaTreeLookup : public ILookupEngine
    {
    public:
        explicit LuleaTreeLookup(const LuleaTree &tree);

        port_t lookup(ipv4_t ip) const override;

        const LuleaTree &tree() const noexcept
        {
            return *this->tree_;
        }

    private:
        const LuleaTree *tree_;
    };

} // namespace engine
