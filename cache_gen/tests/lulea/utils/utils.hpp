#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace tests
{
    namespace fs = std::filesystem;

    struct TableTestFiles
    {
        fs::path dir;
        fs::path bin_path;
        fs::path txt_path;
        std::string name;
    };

    std::vector<TableTestFiles> get_test_tables();

}
