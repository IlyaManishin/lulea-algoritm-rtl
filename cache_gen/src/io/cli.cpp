#include "cli.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace io
{

    void print_usage(const char *prog_name)
    {
        std::cerr << "Usage: " << prog_name << " <config_file.tsv> [--out-csv <results.csv>]\n"
                  << "       " << prog_name << " --batch <config_file.tsv> [--out-csv <results.csv>]\n";
    }

    std::optional<CliOptions> parse_cli_args(std::span<char *const> args)
    {
        if (args.size() < 2)
        {
            return std::nullopt;
        }

        CliOptions options;

        for (size_t i = 1; i < args.size(); ++i)
        {
            const std::string_view arg = args[i];
            if (arg == "--batch" && i + 1 < args.size())
            {
                options.config_file = args[++i];
            }
            else if (arg == "--out-csv" && i + 1 < args.size())
            {
                options.out_csv = args[++i];
            }
            else if (options.config_file.empty() && !arg.starts_with('-'))
            {
                options.config_file = arg;
            }
        }

        if (options.config_file.empty())
        {
            return std::nullopt;
        }

        return options;
    }

} // namespace io
