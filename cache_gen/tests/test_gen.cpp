#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "gen/config.hpp"
#include "gen/gen.hpp"
#include "parser.hpp"

namespace fs = std::filesystem;

void test_gen_interface() {
    std::cout << "[TEST] gen interface & CacheStats struct... ";

    gen::Config cfg(gen::Mode::Stats, "table_001.lrb1");
    assert(cfg.get_mode() == gen::Mode::Stats);
    assert(cfg.get_input_path() == "table_001.lrb1");
    assert(cfg.get_dest_dir().empty());

    cfg.set_dest_dir("out_firmware");
    assert(cfg.get_dest_dir() == "out_firmware");

    gen::BuildResult res = gen::build_lulea_cache(cfg);
    assert(res.is_success());
    assert(res.get_stats().l1_pop == 0);
    assert(res.get_stats().l2_seq == 0);

    std::cout << "PASSED\n";
}

void test_config_parser() {
    std::cout << "[TEST] ConfigParser reading TSV stream... ";

    fs::path temp_tsv = fs::temp_directory_path() / "test_jobs.tsv";
    {
        std::ofstream ofs(temp_tsv);
        ofs << "# Comment line to ignore\n"
            << "\n"
            << "stats\tdata/t1.lrb1\t-\n"
            << "dump\tdata/t2.lrb1\tfirmware/dir/\n";
    }

    ConfigParser parser(temp_tsv.string());

    gen::Config cfg1;
    assert(parser.next(cfg1));
    assert(cfg1.get_mode() == gen::Mode::Stats);
    assert(cfg1.get_input_path() == "data/t1.lrb1");
    assert(cfg1.get_dest_dir().empty());

    gen::Config cfg2;
    assert(parser.next(cfg2));
    assert(cfg2.get_mode() == gen::Mode::Dump);
    assert(cfg2.get_input_path() == "data/t2.lrb1");
    assert(cfg2.get_dest_dir() == "firmware/dir/");

    gen::Config cfg3;
    assert(!parser.next(cfg3)); // EOF

    fs::remove(temp_tsv);
    std::cout << "PASSED\n";
}

int main() {
    try {
        test_gen_interface();
        test_config_parser();
        std::cout << "All tests passed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}
