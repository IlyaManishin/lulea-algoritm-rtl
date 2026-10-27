#pragma once

#include <cstdint>
#include <string>

using ipv4_t = uint32_t;
using mask_t = std::byte;
using port_t = std::byte;

using bin_entry_count_t = uint32_t;

const int HEADER_SIZE = 8;
const std::string_view BIN_MAGIC = "LPB1";
const int ENTRY_SIZE = 8;