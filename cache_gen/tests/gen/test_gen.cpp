#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

#include "gen/gen.hpp"
#include <lulea_common/config.hpp>
#include <lulea_common/reader.hpp>

namespace fs = std::filesystem;
using namespace gen;

class GenTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir = fs::temp_directory_path() / "lulea_gen_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override
    {
        fs::remove_all(test_dir);
    }

    fs::path create_binary_map_file(const std::string &filename,
                                    const std::vector<TMaskEntry> &entries,
                                    std::string_view magic = BIN_MAGIC)
    {
        const fs::path file_path = test_dir / filename;
        std::ofstream ofs(file_path, std::ios::binary);

        ofs.write(magic.data(), static_cast<std::streamsize>(magic.size()));
        const auto count = static_cast<bin_entry_count_t>(entries.size());
        ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));

        for (const auto &entry : entries)
        {
            ofs.write(reinterpret_cast<const char *>(&entry.ip), sizeof(entry.ip));
            ofs.write(reinterpret_cast<const char *>(&entry.mask), sizeof(entry.mask));

            const auto raw_port = std::to_integer<reader_port_t>(entry.port);
            ofs.write(reinterpret_cast<const char *>(&raw_port), sizeof(raw_port));

            constexpr size_t payload_size = sizeof(entry.ip) + sizeof(entry.mask) + sizeof(raw_port);
            constexpr size_t pad_size = ENTRY_SIZE - payload_size;
            const char pad[pad_size] = {0};
            ofs.write(pad, pad_size);
        }

        return file_path;
    }

    fs::path create_raw_entry_file(const std::string &filename,
                                   ipv4_t ip, mask_t mask, reader_port_t raw_port)
    {
        const fs::path file_path = test_dir / filename;
        std::ofstream ofs(file_path, std::ios::binary);

        ofs.write(BIN_MAGIC.data(), BIN_MAGIC.size());
        const bin_entry_count_t count = 1;
        ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));

        ofs.write(reinterpret_cast<const char *>(&ip), sizeof(ip));
        ofs.write(reinterpret_cast<const char *>(&mask), sizeof(mask));
        ofs.write(reinterpret_cast<const char *>(&raw_port), sizeof(raw_port));

        constexpr size_t payload_size = sizeof(ip) + sizeof(mask) + sizeof(raw_port);
        constexpr size_t pad_size = ENTRY_SIZE - payload_size;
        const char pad[pad_size] = {0};
        ofs.write(pad, pad_size);

        return file_path;
    }

    fs::path test_dir;
};

TEST_F(GenTest, EmptyInputPath)
{
    const Config config{
        .mode = Mode::Stats,
        .input_path = "",
        .dest_dir = ""
    };

    EXPECT_THROW(build_lulea_cache(config), FatalGenException);
}

TEST_F(GenTest, DumpWithoutDestDir)
{
    const Config config{
        .mode = Mode::Dump,
        .input_path = "valid_path.bin",
        .dest_dir = ""
    };

    EXPECT_THROW(build_lulea_cache(config), FatalGenException);
}

TEST_F(GenTest, NonExistentFile)
{
    const Config config{
        .mode = Mode::Stats,
        .input_path = (test_dir / "does_not_exist.bin").string(),
        .dest_dir = ""
    };

    try
    {
        build_lulea_cache(config);
        FAIL() << "Expected GenException";
    }
    catch (const FatalGenException &)
    {
        FAIL() << "Expected non-fatal GenException, but got FatalGenException";
    }
    catch (const GenException &ex)
    {
        EXPECT_FALSE(std::string(ex.what()).empty());
    }
}

TEST_F(GenTest, PortOverflow)
{
    const fs::path overflow_file = create_raw_entry_file(
        "overflow.bin",
        0x7F000001,
        static_cast<mask_t>(32),
        300
    );

    const Config config{
        .mode = Mode::Stats,
        .input_path = overflow_file.string(),
        .dest_dir = ""
    };

    try
    {
        build_lulea_cache(config);
        FAIL() << "Expected GenException";
    }
    catch (const FatalGenException &)
    {
        FAIL() << "Expected non-fatal GenException, but got FatalGenException";
    }
    catch (const GenException &ex)
    {
        EXPECT_FALSE(std::string(ex.what()).empty());
    }
}

TEST_F(GenTest, BuildSuccess)
{
    const std::vector<TMaskEntry> entries = {
        {0xC0A80100, static_cast<mask_t>(24), static_cast<port_t>(1)},
        {0x0A000000, static_cast<mask_t>(8), static_cast<port_t>(2)},
        {0x00000000, static_cast<mask_t>(0), static_cast<port_t>(255)}
    };

    const fs::path file_path = create_binary_map_file("valid_routes.bin", entries);
    const Config config{
        .mode = Mode::Stats,
        .input_path = file_path.string(),
        .dest_dir = ""
    };

    BuildResult result;
    EXPECT_NO_THROW({ result = build_lulea_cache(config); });
    (void)result;
}
