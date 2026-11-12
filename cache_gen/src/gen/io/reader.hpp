#pragma once

#include "config.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace gen
{

    struct TMaskEntry
    {
        ipv4_t ip = 0;
        mask_t mask = mask_t{0};
        port_t port = NULL_PORT;
    };

    using MaskEntries = std::vector<TMaskEntry>;

    class PortOverflowError : public std::runtime_error
    {
    public:
        explicit PortOverflowError(const std::string &message)
            : std::runtime_error(message) {}
    };

    MaskEntries *read_mask_mapping(const std::string &map_path);

} // namespace gen
