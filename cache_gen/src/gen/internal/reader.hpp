#pragma once

#include "types.hpp"

#include <stdexcept>
#include <string>

namespace gen
{

    class PortOverflowError : public std::runtime_error
    {
    public:
        explicit PortOverflowError(const std::string &message)
            : std::runtime_error(message) {}
    };

    MaskEntries *read_mask_mapping(const std::string &map_path);

} // namespace gen
