#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "gen/gen.hpp"
#include "io/stats_writer.hpp"

namespace fs = std::filesystem;

class StatsWriterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir = fs::temp_directory_path() / "lulea_stats_writer_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override
    {
        fs::remove_all(test_dir);
    }

    fs::path test_dir;
};

TEST_F(StatsWriterTest, FormatsStdoutCorrectly)
{
    std::ostringstream oss;
    gen::CacheStats stats{
        .l1_pop = 10, .l1_chunk = 20, .l1_seq = 30,
        .l2_pop = 40, .l2_chunk = 50, .l2_seq = 60,
        .l3_pop = 70, .l3_chunk = 80, .l3_seq = 90
    };

    io::StatsWriter::write_stats_stdout(oss, "test_file.lrb1", stats);

    EXPECT_EQ(oss.str(), "test_file.lrb1\t10 20 30 40 50 60 70 80 90\n");
}

TEST_F(StatsWriterTest, FormatsCsvCorrectly)
{
    std::ostringstream oss;
    gen::CacheStats stats{
        .l1_pop = 1, .l1_chunk = 2, .l1_seq = 3,
        .l2_pop = 4, .l2_chunk = 5, .l2_seq = 6,
        .l3_pop = 7, .l3_chunk = 8, .l3_seq = 9
    };

    io::StatsWriter::write_stats_csv(oss, "test_file.lrb1", stats);

    EXPECT_EQ(oss.str(), "test_file.lrb1,1,2,3,4,5,6,7,8,9\n");
}

TEST_F(StatsWriterTest, CustomStreamStdoutMode)
{
    std::ostringstream oss;
    io::StatsWriter writer(oss, false);

    gen::CacheStats stats{
        .l1_pop = 1, .l1_chunk = 2, .l1_seq = 3,
        .l2_pop = 4, .l2_chunk = 5, .l2_seq = 6,
        .l3_pop = 7, .l3_chunk = 8, .l3_seq = 9
    };

    writer.write("data.lrb1", stats);

    EXPECT_EQ(oss.str(), "data.lrb1\t1 2 3 4 5 6 7 8 9\n");
}

TEST_F(StatsWriterTest, CustomStreamCsvModeIncludesHeader)
{
    std::ostringstream oss;
    io::StatsWriter writer(oss, true);

    gen::CacheStats stats{
        .l1_pop = 1, .l1_chunk = 2, .l1_seq = 3,
        .l2_pop = 4, .l2_chunk = 5, .l2_seq = 6,
        .l3_pop = 7, .l3_chunk = 8, .l3_seq = 9
    };

    writer.write("data.lrb1", stats);

    const std::string expected =
        "file,l1_pop,l1_chunk,l1_seq,l2_pop,l2_chunk,l2_seq,l3_pop,l3_chunk,l3_seq\n"
        "data.lrb1,1,2,3,4,5,6,7,8,9\n";

    EXPECT_EQ(oss.str(), expected);
}

TEST_F(StatsWriterTest, WritesToCsvFile)
{
    const fs::path csv_path = test_dir / "output.csv";
    {
        io::StatsWriter writer(csv_path.string());
        EXPECT_TRUE(writer.is_csv());

        gen::CacheStats stats{
            .l1_pop = 11, .l1_chunk = 12, .l1_seq = 13,
            .l2_pop = 21, .l2_chunk = 22, .l2_seq = 23,
            .l3_pop = 31, .l3_chunk = 32, .l3_seq = 33
        };

        gen::Config cfg{gen::Mode::Stats, "input.lrb1", ""};
        writer.write(cfg, stats);
    }

    std::ifstream ifs(csv_path);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    const std::string expected =
        "file,l1_pop,l1_chunk,l1_seq,l2_pop,l2_chunk,l2_seq,l3_pop,l3_chunk,l3_seq\n"
        "input.lrb1,11,12,13,21,22,23,31,32,33\n";

    EXPECT_EQ(content, expected);
}

TEST_F(StatsWriterTest, ThrowsOnInvalidFilePath)
{
    const std::string invalid_path = "/non_existent_dir_12345/non_existent_file.csv";
    EXPECT_THROW(io::StatsWriter writer(invalid_path), std::runtime_error);
}
