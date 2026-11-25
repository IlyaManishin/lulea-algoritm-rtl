#pragma once

#include <lulea_common/config.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace tests
{
    namespace fs = std::filesystem;

    inline constexpr size_t REAL_TABLE_RANDOM_IP_COUNT = 32768;
    inline constexpr size_t REAL_TABLE_STEP_IP_COUNT = 16384;
    // An odd step covers nearly the full IPv4 range and varies the low address bits.
    inline constexpr gen::ipv4_t REAL_TABLE_IP_STEP = 262143U;

    struct TableTestFiles
    {
        fs::path dir;
        fs::path bin_path;
        fs::path txt_path;
        std::string name;
    };

    std::vector<TableTestFiles> get_test_tables();

}
