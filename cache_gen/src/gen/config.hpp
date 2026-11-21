#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace gen
{
    using ipv4_t = uint32_t;
    using mask_t = std::byte;
    using port_t = std::byte;
    using reader_port_t = uint16_t;

    using bin_entry_count_t = uint32_t;
    
    inline constexpr size_t IP_SIZE_BITS = sizeof(ipv4_t) * 8;
    inline constexpr size_t HEADER_SIZE = 8;
    inline constexpr size_t ENTRY_SIZE = 8;
    inline constexpr std::string_view BIN_MAGIC = "LRB1";
    
    inline constexpr port_t NULL_PORT = static_cast<port_t>(0);

} // namespace gen
