#include "gen/gen.hpp"
#include "io/cli.hpp"
#include "io/config_parser.hpp"
#include "io/progress_bar.hpp"
#include "io/stats_writer.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>

using namespace io;

namespace
{
    enum class EntryResult : std::uint8_t
    {
        Success,
        Failed,
        Fatal
    };

    EntryResult process_config_entry(const gen::Config &config, const StatsWriter &writer,
                                     ProgressBar &progress)
    {
        try
        {
            const gen::BuildResult result = gen::build_lulea_cache(config);
            writer.write(config.input_path, result.stats);
            return EntryResult::Success;
        }
        catch (const gen::FatalGenException &ex)
        {
            progress.end_line();
            std::cerr << "Fatal build error for " << config.input_path
                      << ": " << ex.what() << "\n";
            return EntryResult::Fatal;
        }
        catch (const gen::GenException &ex)
        {
            progress.end_line();
            std::cerr << "Build error for " << config.input_path
                      << ": " << ex.what() << "\n";
            return EntryResult::Failed;
        }
        catch (const std::exception &ex)
        {
            progress.end_line();
            std::cerr << "Unexpected error for " << config.input_path
                      << ": " << ex.what() << "\n";
            return EntryResult::Fatal;
        }
    }

    int process_config(const CliOptions &opt)
    {
        ConfigParser parser(opt.config_file);

        std::size_t total_jobs = 0;
        if (opt.progress)
        {
            total_jobs = parser.count_remaining();
        }
        const bool redraw_progress = !opt.out_csv.empty();
        ProgressBar progress(total_jobs, redraw_progress);
        
        const StatsWriter writer(opt.out_csv);
        gen::Config config;
        int status = 0;
        while (parser.next(config))
        {
            const EntryResult result = process_config_entry(config, writer, progress);
            if (result == EntryResult::Fatal)
            {
                std::cerr << "Fatal error encountered. Aborting further processing.\n";
                return 1;
            }
            progress.advance();
            if (result == EntryResult::Failed)
            {
                status = 1;
            }
        }
        return status;
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
            return process_config(options.value());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Unexpected error: " << e.what() << "\n";
            return 1;
        }
    }

} // namespace

int main(int argc, char *argv[])
{
    const std::span<char *const> args(argv, static_cast<size_t>(argc));
    return run_program(args);
}
