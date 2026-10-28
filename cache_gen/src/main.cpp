#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "gen/gen.hpp"
#include "parser.hpp"

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

    std::optional<CliOptions> parse_cli_args(int argc, char *argv[])
    {
        if (argc < 2)
        {
            return std::nullopt;
        }

        CliOptions options;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--batch" && i + 1 < argc)
            {
                options.config_file = argv[++i];
            }
            else if (arg == "--out-csv" && i + 1 < argc)
            {
                options.out_csv = argv[++i];
            }
            else if (options.config_file.empty() && arg[0] != '-')
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
        gen::BuildResult result = gen::build_lulea_cache(config);

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
    auto options = parse_cli_args(argc, argv);
    if (!options.has_value())
    {
        print_usage(argv[0]);
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
