#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "gen/gen.hpp"
#include "parser.hpp"

namespace fs = std::filesystem;

class ConfigParserTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir = fs::temp_directory_path() / "lulea_parser_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override
    {
        fs::remove_all(test_dir);
    }

    fs::path create_manifest_file(const std::string &filename, const std::string &content)
    {
        fs::path file_path = test_dir / filename;
        std::ofstream ofs(file_path);
        ofs << content;
        return file_path;
    }

    fs::path test_dir;
};

TEST_F(ConfigParserTest, ReadsValidTsvStream)
{
    const std::string tsv_content =
        "# Header comment line\n"
        "\n"
        "stats\tdata/table_1.lrb1\t-\n"
        "dump\tdata/table_2.lrb1\tfirmware/dir/\n";

    fs::path manifest = create_manifest_file("jobs.tsv", tsv_content);
    ConfigParser parser(manifest.string());

    gen::Config cfg1;
    ASSERT_TRUE(parser.next(cfg1));
    EXPECT_EQ(cfg1.mode, gen::Mode::Stats);
    EXPECT_EQ(cfg1.input_path, "data/table_1.lrb1");
    EXPECT_EQ(cfg1.dest_dir, "");

    gen::Config cfg2;
    ASSERT_TRUE(parser.next(cfg2));
    EXPECT_EQ(cfg2.mode, gen::Mode::Dump);
    EXPECT_EQ(cfg2.input_path, "data/table_2.lrb1");
    EXPECT_EQ(cfg2.dest_dir, "firmware/dir/");

    gen::Config cfg3;
    EXPECT_FALSE(parser.next(cfg3));
}

TEST_F(ConfigParserTest, ThrowsOnMalformedEntry)
{
    const std::string bad_content = "invalid_line_without_tabs\n";
    fs::path manifest = create_manifest_file("bad_jobs.tsv", bad_content);
    ConfigParser parser(manifest.string());

    gen::Config cfg;
    EXPECT_THROW(parser.next(cfg), std::runtime_error);
}
