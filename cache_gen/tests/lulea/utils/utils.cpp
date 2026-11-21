#include "utils/utils.hpp"

#include "utils/test_env.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace tests
{
    namespace
    {
        size_t extract_table_number(const std::string &name)
        {
            if (name.rfind("table_", 0) == 0)
            {
                try
                {
                    return std::stoull(name.substr(6));
                }
                catch (...)
                {
                }
            }
            return 0;
        }
    }

    std::vector<TableTestFiles> get_test_tables()
    {
        const fs::path tables_root = get_tables_root_dir();
        if (!fs::is_directory(tables_root))
        {
            return {};
        }

        std::vector<TableTestFiles> tables;
        for (const auto &entry : fs::directory_iterator(tables_root))
        {
            if (!entry.is_directory())
            {
                continue;
            }

            const fs::path table_dir = entry.path();
            const fs::path bin_path = table_dir / "routes.bin";
            const fs::path txt_path = table_dir / "routes.txt";

            if (fs::is_regular_file(bin_path) && fs::is_regular_file(txt_path))
            {
                tables.push_back(TableTestFiles{
                    .dir = table_dir,
                    .bin_path = bin_path,
                    .txt_path = txt_path,
                    .name = table_dir.filename().string(),
                });
            }
        }

        std::sort(tables.begin(), tables.end(), [](const TableTestFiles &lhs, const TableTestFiles &rhs) {
            const size_t num_lhs = extract_table_number(lhs.name);
            const size_t num_rhs = extract_table_number(rhs.name);
            if (num_lhs != num_rhs)
            {
                return num_lhs < num_rhs;
            }
            return lhs.name < rhs.name;
        });

        return tables;
    }

}
