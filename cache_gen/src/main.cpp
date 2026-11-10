#include "cli/parser.hpp"
#include "gen/gen.hpp"

#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

namespace
{

    struct CliOptions
    {
        std::string config_file;
        std::string out_csv;
    };

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

    void print_stats_stdout(const std::string &input_path, const gen::CacheStats &stats)
    {
        std::cout << input_path << "\t"
                  << stats.l1_pop << " " << stats.l1_chunk << " " << stats.l1_seq << " "
                  << stats.l2_pop << " " << stats.l2_chunk << " " << stats.l2_seq << " "
                  << stats.l3_pop << " " << stats.l3_chunk << " " << stats.l3_seq << "\n";
    }

    void write_stats_csv(std::ostream &out, const std::string &input_path, const gen::CacheStats &stats)
    {
        out << input_path << ","
            << stats.l1_pop << "," << stats.l1_chunk << "," << stats.l1_seq << ","
            << stats.l2_pop << "," << stats.l2_chunk << "," << stats.l2_seq << ","
            << stats.l3_pop << "," << stats.l3_chunk << "," << stats.l3_seq << "\n";
    }

    void process_config_entry(const gen::Config &config, std::ofstream &csv_file)
    {
        const gen::BuildResult result = gen::build_lulea_cache(config);

        if (!result.success)
        {
            std::cerr << "Build error for " << config.input_path
                      << ": " << result.error_message << "\n";
            return;
        }

        const auto &stats = result.stats;

        if (csv_file.is_open())
        {
            write_stats_csv(csv_file, config.input_path, stats);
        }
        else
        {
            print_stats_stdout(config.input_path, stats);
        }
    }

    int process_manifest(const CliOptions &options)
    {
        ConfigParser parser(options.config_file);
        std::ofstream csv_file;

        if (!options.out_csv.empty())
        {
            csv_file.open(options.out_csv);
            if (!csv_file.is_open())
            {
                std::cerr << "Error: cannot open output CSV: " << options.out_csv << "\n";
                return 1;
            }
            csv_file << "file,l1_pop,l1_chunk,l1_seq,l2_pop,l2_chunk,l2_seq,l3_pop,l3_chunk,l3_seq\n";
        }

        gen::Config config;
        while (parser.next(config))
        {
            process_config_entry(config, csv_file);
        }

        return 0;
    }

} // anonymous namespace

int main(int argc, char *argv[])
{
    const std::span<char *const> args(argv, static_cast<size_t>(argc));
    const auto options = parse_cli_args(args);
    if (!options.has_value())
    {
        print_usage(args.empty() ? "lulea_builder" : args.front());
        return 1;
    }

    try
    {
        return process_manifest(options.value());
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
