#include "utils.hpp"

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tests
{
    namespace
    {
        void require_fixture(const fs::path &path)
        {
            if (!fs::is_regular_file(path))
            {
                throw std::runtime_error("Missing routing table fixture: " + fs::absolute(path).string());
            }
        }
    } // namespace

    std::vector<TableTestFiles> get_test_tables()
    {
        // Meson runs the verification tests next to the generated tables directory.
        const fs::path tables_root = "tables";
        constexpr size_t table_count = LULEA_VERIFY_TABLE_COUNT;
        std::vector<TableTestFiles> tables;
        tables.reserve(table_count);
        for (size_t number = 1; number <= table_count; ++number)
        {
            const std::string name = "table_" + std::to_string(number);
            const fs::path table_dir = tables_root / name;
            TableTestFiles table{
                .dir = table_dir,
                .bin_path = table_dir / "routes.bin",
                .txt_path = table_dir / "routes.txt",
                .name = name,
            };
            require_fixture(table.bin_path);
            require_fixture(table.txt_path);
            tables.push_back(std::move(table));
        }

        return tables;
    }

}
