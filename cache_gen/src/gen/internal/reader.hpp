#pragma once

#include <string>
#include "types.hpp"

namespace gen
{

MaskEntries *read_mask_mapping(const std::string &map_path);

} // namespace gen
