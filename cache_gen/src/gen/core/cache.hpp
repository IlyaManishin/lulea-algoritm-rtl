#pragma once

#include "gen/config.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

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

    class BaseCell
    {
    public:
        explicit BaseCell(size_t cell_size_bits, size_t default_value = 0);

        void set_raw(size_t raw_val);
        size_t raw() const noexcept { return this->data_; }

        size_t bit_width() const noexcept { return this->cell_size_bits_; }
        size_t max_raw() const noexcept;

        friend bool operator==(const BaseCell &lhs, const BaseCell &rhs) noexcept = default;

    protected:
        static size_t mask_for_bits(size_t bits) noexcept;

    private:
        size_t data_{0};
        size_t cell_size_bits_;
    };

    class LuleaCell : public BaseCell
    {
    public:
        explicit LuleaCell(size_t cell_size_bits, port_t default_port = NULL_PORT);

        static LuleaCell make_port(size_t cell_size_bits, port_t port_val);
        static LuleaCell make_ptr(size_t cell_size_bits, size_t ptr_val);

        void set_port(port_t port_val);
        void set_ptr(size_t ptr_val);

        bool is_ptr() const noexcept;
        bool is_port() const noexcept;
        bool is_null() const noexcept;

        size_t value() const noexcept;
        port_t port() const;
        size_t ptr() const;

        size_t max_value() const noexcept;

    private:
        size_t flag_bit_mask() const noexcept;
        size_t payload_mask() const noexcept;
    };

    class LuleaRoute
    {
    public:
        using value_type = LuleaCell;
        using iterator = std::vector<LuleaCell>::iterator;
        using const_iterator = std::vector<LuleaCell>::const_iterator;

        LuleaRoute(size_t size, size_t cell_size_bits, port_t default_port = NULL_PORT);

        LuleaCell &operator[](size_t index) { return this->data_[index]; }
        const LuleaCell &operator[](size_t index) const { return this->data_[index]; }

        size_t size() const noexcept { return this->data_.size(); }
        bool empty() const noexcept { return this->data_.empty(); }
        size_t cell_size_bits() const noexcept { return this->cell_size_bits_; }

        iterator begin() noexcept { return this->data_.begin(); }
        iterator end() noexcept { return this->data_.end(); }
        const_iterator begin() const noexcept { return this->data_.begin(); }
        const_iterator end() const noexcept { return this->data_.end(); }
        const_iterator cbegin() const noexcept { return this->data_.cbegin(); }
        const_iterator cend() const noexcept { return this->data_.cend(); }

    private:
        std::vector<LuleaCell> data_;
        size_t cell_size_bits_;
    };

} // namespace gen
