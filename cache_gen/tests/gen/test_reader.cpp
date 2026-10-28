#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "gen/config.hpp"
#include "gen/internal/reader.hpp"
#include "gen/types.hpp"

namespace fs = std::filesystem;

class ReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / "lulea_reader_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        fs::remove_all(test_dir);
    }

    fs::path create_empty_file(const std::string& filename) {
        fs::path file_path = test_dir / filename;
        std::ofstream ofs(file_path, std::ios::binary);
        return file_path;
    }

    fs::path create_binary_map_file(const std::string& filename,
                                    const std::vector<TMaskEntry>& entries,
                                    std::string_view magic = BIN_MAGIC) {
        fs::path file_path = test_dir / filename;
        std::ofstream ofs(file_path, std::ios::binary);

        // Header: 4 bytes magic signature + 4 bytes entry count
        ofs.write(magic.data(), magic.size());
        bin_entry_count_t count = static_cast<bin_entry_count_t>(entries.size());
        ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));

        // Entries: each record is ENTRY_SIZE (8) bytes:
        // ipv4_t (4B) + mask_t (1B) + reader_port_t (2B) + pad (1B)
        for (const auto& entry : entries) {
            ofs.write(reinterpret_cast<const char*>(&entry.ip), sizeof(entry.ip));
            ofs.write(reinterpret_cast<const char*>(&entry.mask), sizeof(entry.mask));

            reader_port_t raw_port = std::to_integer<reader_port_t>(entry.port);
            ofs.write(reinterpret_cast<const char*>(&raw_port), sizeof(raw_port));

            constexpr size_t payload_size = sizeof(entry.ip) + sizeof(entry.mask) + sizeof(raw_port);
            constexpr size_t pad_size = ENTRY_SIZE - payload_size;
            const char pad[pad_size] = {0};
            ofs.write(pad, pad_size);
        }

        return file_path;
    }

    fs::path create_raw_entry_file(const std::string& filename,
                                   ipv4_t ip, mask_t mask, reader_port_t raw_port) {
        fs::path file_path = test_dir / filename;
        std::ofstream ofs(file_path, std::ios::binary);

        ofs.write(BIN_MAGIC.data(), BIN_MAGIC.size());
        bin_entry_count_t count = 1;
        ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));

        ofs.write(reinterpret_cast<const char*>(&ip), sizeof(ip));
        ofs.write(reinterpret_cast<const char*>(&mask), sizeof(mask));
        ofs.write(reinterpret_cast<const char*>(&raw_port), sizeof(raw_port));

        constexpr size_t payload_size = sizeof(ip) + sizeof(mask) + sizeof(raw_port);
        constexpr size_t pad_size = ENTRY_SIZE - payload_size;
        const char pad[pad_size] = {0};
        ofs.write(pad, pad_size);

        return file_path;
    }

    fs::path test_dir;
};

// =========================================================================
//  Reader Tests
// =========================================================================

TEST_F(ReaderTest, ThrowsOnNonExistentFile) {
    EXPECT_THROW(
        gen::read_mask_mapping((test_dir / "does_not_exist.bin").string()),
        std::runtime_error
    );
}

TEST_F(ReaderTest, ThrowsOnEmptyFile) {
    fs::path empty_file = create_empty_file("empty.bin");
    EXPECT_THROW(
        gen::read_mask_mapping(empty_file.string()),
        std::runtime_error
    );
}

TEST_F(ReaderTest, ReadsZeroEntries) {
    fs::path zero_file = create_binary_map_file("zero_entries.bin", {});
    std::unique_ptr<MaskEntries> entries(gen::read_mask_mapping(zero_file.string()));

    ASSERT_NE(entries, nullptr);
    EXPECT_TRUE(entries->empty());
    EXPECT_EQ(entries->size(), 0u);
}

TEST_F(ReaderTest, ReadsMultipleEntries) {
    const std::vector<TMaskEntry> expected = {
        {0xC0A80100, static_cast<mask_t>(24), static_cast<port_t>(1)},
        {0x0A000000, static_cast<mask_t>(8),  static_cast<port_t>(2)},
        {0xAC100000, static_cast<mask_t>(16), static_cast<port_t>(3)},
        {0x00000000, static_cast<mask_t>(0),  static_cast<port_t>(255)}
    };

    fs::path file_path = create_binary_map_file("multiple_entries.bin", expected);
    std::unique_ptr<MaskEntries> entries(gen::read_mask_mapping(file_path.string()));

    ASSERT_NE(entries, nullptr);
    ASSERT_EQ(entries->size(), expected.size());

    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ((*entries)[i].ip, expected[i].ip);
        EXPECT_EQ((*entries)[i].mask, expected[i].mask);
        EXPECT_EQ((*entries)[i].port, expected[i].port);
    }
}

TEST_F(ReaderTest, ThrowsOnInvalidMagicSignature) {
    fs::path bad_magic_file = create_binary_map_file("bad_magic.bin", {}, "XXXX");
    EXPECT_THROW(
        gen::read_mask_mapping(bad_magic_file.string()),
        std::runtime_error
    );
}

TEST_F(ReaderTest, ThrowsOnPortOverflowInFile) {
    fs::path overflow_file = create_raw_entry_file(
        "overflow.bin",
        0x7F000001,
        static_cast<mask_t>(32),
        300 // Exceeds 8-bit port_t
    );

    EXPECT_THROW(
        gen::read_mask_mapping(overflow_file.string()),
        gen::PortOverflowError
    );
}
