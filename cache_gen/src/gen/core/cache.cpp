#include "cache.hpp"

#include "gen/config.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace gen
{

    namespace
    {
        constexpr size_t kMinBaseCellSizeBits = 1;
        constexpr size_t kMinLuleaCellSizeBits = 2;
        constexpr size_t kMaxCellSizeBits = static_cast<size_t>(std::numeric_limits<size_t>::digits);
    } // namespace

    BaseCell::BaseCell(size_t cell_size_bits, size_t default_value)
        : cell_size_bits_(cell_size_bits)
    {
        if (this->cell_size_bits_ < kMinBaseCellSizeBits || this->cell_size_bits_ > kMaxCellSizeBits)
        {
            throw std::invalid_argument(
                "cell_size_bits must be between " + std::to_string(kMinBaseCellSizeBits) +
                " and " + std::to_string(kMaxCellSizeBits));
        }

        this->set_raw(default_value);
    }

    size_t BaseCell::mask_for_bits(size_t bits) noexcept
    {
        if (bits == 0)
        {
            return 0;
        }
        if (bits >= kMaxCellSizeBits)
        {
            return std::numeric_limits<size_t>::max();
        }
        return (size_t{1} << bits) - 1;
    }

    size_t BaseCell::max_raw() const noexcept
    {
        return mask_for_bits(this->cell_size_bits_);
    }

    void BaseCell::set_raw(size_t raw_val)
    {
        const size_t max_val = this->max_raw();
        if (raw_val > max_val)
        {
            throw LuleaOverflow(
                "Value " + std::to_string(raw_val) +
                " exceeds BaseCell capacity " + std::to_string(max_val));
        }

        this->data_ = raw_val;
    }

    LuleaCell::LuleaCell(size_t cell_size_bits, port_t default_port)
        : BaseCell(cell_size_bits)
    {
        if (this->bit_width() < kMinLuleaCellSizeBits)
        {
            throw std::invalid_argument(
                "LuleaCell cell_size_bits must be between " + std::to_string(kMinLuleaCellSizeBits) +
                " and " + std::to_string(kMaxCellSizeBits));
        }

        this->set_port(default_port);
    }

    LuleaCell LuleaCell::make_port(size_t cell_size_bits, port_t port_val)
    {
        return LuleaCell(cell_size_bits, port_val);
    }

    LuleaCell LuleaCell::make_ptr(size_t cell_size_bits, size_t ptr_val)
    {
        LuleaCell cell(cell_size_bits);
        cell.set_ptr(ptr_val);
        return cell;
    }

    size_t LuleaCell::flag_bit_mask() const noexcept
    {
        return size_t{1} << (this->bit_width() - 1);
    }

    size_t LuleaCell::payload_mask() const noexcept
    {
        return mask_for_bits(this->bit_width() - 1);
    }

    size_t LuleaCell::max_value() const noexcept
    {
        return this->payload_mask();
    }

    void LuleaCell::set_port(port_t port_val)
    {
        const auto raw_port = std::to_integer<size_t>(port_val);
        const size_t max_val = this->max_value();
        if (raw_port > max_val)
        {
            throw LuleaOverflow(
                "Port value " + std::to_string(raw_port) +
                " exceeds LuleaCell max payload " + std::to_string(max_val));
        }

        this->set_raw(raw_port);
    }

    void LuleaCell::set_ptr(size_t ptr_val)
    {
        const size_t max_val = this->max_value();
        if (ptr_val > max_val)
        {
            throw LuleaOverflow(
                "Pointer value " + std::to_string(ptr_val) +
                " exceeds LuleaCell max payload " + std::to_string(max_val));
        }

        this->set_raw(ptr_val | this->flag_bit_mask());
    }

    bool LuleaCell::is_ptr() const noexcept
    {
        return (this->raw() & this->flag_bit_mask()) != 0;
    }

    bool LuleaCell::is_port() const noexcept
    {
        return !this->is_ptr();
    }

    bool LuleaCell::is_null() const noexcept
    {
        return this->value() == std::to_integer<size_t>(NULL_PORT);
    }

    size_t LuleaCell::value() const noexcept
    {
        return this->raw() & this->payload_mask();
    }

    port_t LuleaCell::port() const
    {
        if (this->is_ptr())
        {
            throw std::logic_error("LuleaCell holds a pointer, not a port");
        }

        const size_t val = this->value();
        if (val > static_cast<size_t>(std::numeric_limits<uint8_t>::max()))
        {
            throw LuleaOverflow("Stored value exceeds port_t range");
        }

        return static_cast<port_t>(static_cast<uint8_t>(val));
    }

    size_t LuleaCell::ptr() const
    {
        if (!this->is_ptr())
        {
            throw std::logic_error("LuleaCell holds a port, not a pointer");
        }

        return this->value();
    }

    LuleaRoute::LuleaRoute(size_t size, size_t cell_size_bits, port_t default_port)
        : data_(size, LuleaCell(cell_size_bits, default_port)),
          cell_size_bits_(cell_size_bits)
    {
    }

} // namespace gen
