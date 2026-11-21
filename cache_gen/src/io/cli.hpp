#pragma once

#include <optional>
#include <span>
#include <string>

namespace io
{

    struct CliOptions
    {
        std::string config_file;
        std::string out_csv;
    };

    void print_usage(const char *prog_name);

    std::optional<CliOptions> parse_cli_args(std::span<char *const> args);

} // namespace io
