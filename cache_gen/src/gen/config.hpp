#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

using ipv4_t = uint32_t;
using mask_t = std::byte;
using port_t = std::byte;
using reader_port_t = uint16_t;

using bin_entry_count_t = uint32_t;

const size_t HEADER_SIZE = 8;
const size_t ENTRY_SIZE = 8;
const std::string_view BIN_MAGIC = "LRB1";