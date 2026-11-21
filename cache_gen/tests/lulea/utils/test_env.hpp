#pragma once

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <vector>

namespace tests
{
    namespace fs = std::filesystem;

    // Compile-time path and directory name constants
    constexpr std::string_view TABLES_DIR_NAME = "tables";
    constexpr std::string_view REL_TABLES_DIR = "tests/lulea/tables";

    inline fs::path get_tables_root_dir()
    {
        // 1. Resolve relative to this header file's location
        // test_env.hpp is located at <project_root>/cache_gen/tests/lulea/utils/test_env.hpp
        // utils_dir -> .../tests/lulea/utils
        // utils_dir.parent_path() -> .../tests/lulea
        // tables_dir -> .../tests/lulea/tables
        const fs::path utils_dir = fs::path(__FILE__).parent_path();
        const fs::path tables_dir = utils_dir.parent_path() / TABLES_DIR_NAME;
        if (fs::is_directory(tables_dir))
        {
            return tables_dir;
        }

        // 2. Fallback relative paths if executed from different working directories
        const std::vector<fs::path> candidates = {
            REL_TABLES_DIR,
            "../tests/lulea/tables",
            "cache_gen/tests/lulea/tables",
        };
        const auto it_cand = std::find_if(candidates.begin(), candidates.end(), [](const fs::path &cand) {
            return fs::is_directory(cand);
        });
        if (it_cand != candidates.end())
        {
            return *it_cand;
        }

        return tables_dir;
    }

} // namespace tests
