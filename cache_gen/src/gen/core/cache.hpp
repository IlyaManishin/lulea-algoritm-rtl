#pragma once

#include "config.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace gen
{

    using lulea_cell_t = uint32_t;

    class LuleaRoute
    {
    private:
        std::vector<lulea_cell_t> data;
        size_t cell_size_bits;

    public:
        LuleaRoute(size_t size, size_t cell_size_bits, port_t default_port = NULL_PORT);

        // need a iterator for data;

        lulea_cell_t operator[](size_t index) { return data[index]; };
        bool is_ptr(lulea_cell_t cell) { return (cell >> (this->cell_size_bits - 1)) & 1; };
        size_t cell_to_val(lulea_cell_t cell) { return cell & ((1 << this->cell_size_bits) - 1); };

        void add_port(port_t port);
        void add_ptr(size_t ptr);
        

        ~LuleaRoute();
    };

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
