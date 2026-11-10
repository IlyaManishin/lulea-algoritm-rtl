#include "gen/gen.hpp"
#include "io/cli.hpp"
#include "io/config_parser.hpp"
#include "io/stats_writer.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <span>

using namespace io;

namespace
{
    void process_config_entry(const gen::Config &config, const StatsWriter &writer)
    {
        const gen::BuildResult result = gen::build_lulea_cache(config);

        if (!result.success)
        {
            std::cerr << "Build error for " << config.input_path
                      << ": " << result.error_message << "\n";
            return;
        }

        writer.write(config.input_path, result.stats);
    }

    void process_config(const CliOptions &opt)
    {
        ConfigParser parser(opt.config_file);
        const StatsWriter writer(opt.out_csv);

        gen::Config config;
        while (parser.next(config))
        {
            process_config_entry(config, writer);
        }
    }

    int run_program(std::span<char *const> args)
    {
        const auto options = parse_cli_args(args);
        if (!options.has_value())
        {
            const char *prog_name = (args.empty() || args.front() == nullptr) ? "lulea_builder" : args.front();
            print_usage(prog_name);
            return 1;
        }

        try
        {
            process_config(options.value());
            return 0;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

} // namespace

int main(int argc, char *argv[])
{
    const std::span<char *const> args(argv, static_cast<size_t>(argc));
    return run_program(args);
}
