#include <iostream>
#include <fstream>
#include <string>

#include "gen/config.hpp"
#include "gen/gen.hpp"
#include "parser.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file.tsv> [--out-csv <results.csv>]\n"
                  << "       " << argv[0] << " --batch <config_file.tsv> [--out-csv <results.csv>]\n";
        return 1;
    }

    std::string config_file;
    std::string out_csv;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--batch" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "--out-csv" && i + 1 < argc) {
            out_csv = argv[++i];
        } else if (config_file.empty() && arg[0] != '-') {
            config_file = arg;
        }
    }

    if (config_file.empty()) {
        std::cerr << "Error: No config file specified\n";
        return 1;
    }

    try {
        ConfigParser parser(config_file);
        std::ofstream csv_file;
        if (!out_csv.empty()) {
            csv_file.open(out_csv);
            if (!csv_file.is_open()) {
                std::cerr << "Error: cannot open output CSV: " << out_csv << "\n";
                return 1;
            }
            csv_file << "file,l1_pop,l1_chunk,l1_seq,l2_pop,l2_chunk,l2_seq,l3_pop,l3_chunk,l3_seq\n";
        }

        gen::Config config;
        while (parser.next(config)) {
            gen::BuildResult result = gen::build_lulea_cache(config);

            if (!result.is_success()) {
                std::cerr << "Build error for " << config.get_input_path()
                          << ": " << result.get_error_message() << "\n";
                continue;
            }

            const auto& stats = result.get_stats();

            if (csv_file.is_open()) {
                csv_file << config.get_input_path() << ","
                         << stats.l1_pop << "," << stats.l1_chunk << "," << stats.l1_seq << ","
                         << stats.l2_pop << "," << stats.l2_chunk << "," << stats.l2_seq << ","
                         << stats.l3_pop << "," << stats.l3_chunk << "," << stats.l3_seq << "\n";
            } else {
                std::cout << config.get_input_path() << "\t"
                          << stats.l1_pop << " " << stats.l1_chunk << " " << stats.l1_seq << " "
                          << stats.l2_pop << " " << stats.l2_chunk << " " << stats.l2_seq << " "
                          << stats.l3_pop << " " << stats.l3_chunk << " " << stats.l3_seq << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
