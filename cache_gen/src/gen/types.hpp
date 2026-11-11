#pragma once

#include "config.hpp"

#include <vector>

namespace gen
{

    struct TMaskEntry
    {
        ipv4_t ip = 0;
        mask_t mask = mask_t{0};
        port_t port = port_t{0};
    };

    using MaskEntries = std::vector<TMaskEntry>;

} // namespace gen
