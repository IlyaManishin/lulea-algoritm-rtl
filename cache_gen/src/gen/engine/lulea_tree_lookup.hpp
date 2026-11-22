#pragma once

#include "config.hpp"
#include "lookup_engine.hpp"
#include "lulea_algo/lulea_tree.hpp"

namespace engine
{
    using namespace gen;

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
