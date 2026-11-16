#pragma once

#include <stdexcept>
#include <string>

namespace gen
{
    class LuleaOverflow : public std::overflow_error
    {
    public:
        LuleaOverflow()
            : std::overflow_error("Lulea table capacity exceeded") {}

        explicit LuleaOverflow(const std::string &message)
            : std::overflow_error(message) {}

        explicit LuleaOverflow(const char *message)
            : std::overflow_error(message) {}
    };

} // namespace gen
