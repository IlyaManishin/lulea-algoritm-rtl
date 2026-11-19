#pragma once

#include "config.hpp"
#include <vector>

typedef struct TMaskEntry
{
    ipv4_t ip;
    mask_t mask;
    port_t port;
} TMaskEntry;

using MaskEntries = std::vector<TMaskEntry>;
