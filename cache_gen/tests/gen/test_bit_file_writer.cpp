#include <gtest/gtest.h>

#include "gen/io/bit_file_writer.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

class BitFileWriterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir = fs::temp_directory_path() / "lulea_bit_file_writer_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override
    {
        fs::remove_all(test_dir);
    }

    std::vector<uint8_t> read_binary_file(const fs::path &file_path) const
    {
        std::ifstream ifs(file_path, std::ios::binary);
        return {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    }

    fs::path test_dir;
};

TEST_F(BitFileWriterTest, ThrowsOnInvalidPath)
{
    const fs::path invalid_path = "/non_existent_dir_12345/sub/file.bin";
    EXPECT_THROW(gen::BitFileWriter writer(invalid_path), std::runtime_error);
}

TEST_F(BitFileWriterTest, WritesExactBytes)
{
    const fs::path file_path = test_dir / "exact_bytes.bin";
    {
        gen::BitFileWriter writer(file_path);
        writer.write_bits(0xAB, 8);
        writer.write_bits(0xCD, 8);
        EXPECT_EQ(writer.total_bits_written(), 16);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 0xAB);
    EXPECT_EQ(data[1], 0xCD);
}

TEST_F(BitFileWriterTest, PadsPartialByteOnClose)
{
    const fs::path file_path = test_dir / "padded.bin";
    {
        gen::BitFileWriter writer(file_path);
        // Write 3 bits: 0b101 (5)
        writer.write_bits(0b101, 3);
        EXPECT_EQ(writer.total_bits_written(), 3);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 1);
    // 3 bits in LSB, remaining 5 bits padded with 0
    EXPECT_EQ(data[0], 0b00000101);
}

TEST_F(BitFileWriterTest, BitPackingAcrossByteBoundaries)
{
    const fs::path file_path = test_dir / "bit_packing.bin";
    {
        gen::BitFileWriter writer(file_path);
        // 4 bits: 0b1010 (0x0A)
        writer.write_bits(0x0A, 4);
        // 6 bits: 0b110011 (0x33)
        writer.write_bits(0x33, 6);
        EXPECT_EQ(writer.total_bits_written(), 10);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 2);
    // Byte 0: bits 0..3 are 0x0A, bits 4..7 are lowest 4 bits of 0x33 (0x03) -> 0x3A
    EXPECT_EQ(data[0], 0x3A);
    // Byte 1: remaining 2 bits of 0x33 (0b11 = 0x03), padded with 0 -> 0x03
    EXPECT_EQ(data[1], 0x03);
}

TEST_F(BitFileWriterTest, Writes64BitValues)
{
    const fs::path file_path = test_dir / "64bit.bin";
    {
        gen::BitFileWriter writer(file_path);
        writer.write_bits(0x0123456789ABCDEFULL, 64);
        EXPECT_EQ(writer.total_bits_written(), 64);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 8);
    // Little-endian byte order as accumulator emits lowest 8 bits first
    EXPECT_EQ(data[0], 0xEF);
    EXPECT_EQ(data[1], 0xCD);
    EXPECT_EQ(data[2], 0xAB);
    EXPECT_EQ(data[3], 0x89);
    EXPECT_EQ(data[4], 0x67);
    EXPECT_EQ(data[5], 0x45);
    EXPECT_EQ(data[6], 0x23);
    EXPECT_EQ(data[7], 0x01);
}

TEST_F(BitFileWriterTest, ZeroBitsIsNoOp)
{
    const fs::path file_path = test_dir / "zero_bits.bin";
    {
        gen::BitFileWriter writer(file_path);
        writer.write_bits(0xFFFF, 0);
        EXPECT_EQ(writer.total_bits_written(), 0);
    }

    const auto data = read_binary_file(file_path);
    EXPECT_TRUE(data.empty());
}

TEST_F(BitFileWriterTest, WriteBytesSpanFull)
{
    const fs::path file_path = test_dir / "bytes_full.bin";
    const std::vector<std::byte> input = {
        std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};

    {
        gen::BitFileWriter writer(file_path);
        writer.write_bytes(input);
        EXPECT_EQ(writer.total_bits_written(), 32);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 4);
    EXPECT_EQ(data[0], 0xDE);
    EXPECT_EQ(data[1], 0xAD);
    EXPECT_EQ(data[2], 0xBE);
    EXPECT_EQ(data[3], 0xEF);
}

TEST_F(BitFileWriterTest, WriteBytesSpanPartialBits)
{
    const fs::path file_path = test_dir / "bytes_partial.bin";
    const std::vector<std::byte> input = {
        std::byte{0xFF}, std::byte{0xFF}};

    {
        gen::BitFileWriter writer(file_path);
        // Only 12 bits: byte 0 (8 bits) + lower 4 bits of byte 1
        writer.write_bytes(input, 12);
        EXPECT_EQ(writer.total_bits_written(), 12);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 0xFF);
    EXPECT_EQ(data[1], 0x0F);
}

TEST_F(BitFileWriterTest, BufferFlushWhenExceedingCapacity)
{
    const fs::path file_path = test_dir / "small_buffer.bin";
    constexpr size_t small_buf = 4;

    {
        gen::BitFileWriter writer(file_path, small_buf);
        for (uint8_t i = 0; i < 10; ++i)
        {
            writer.write_bits(i, 8);
        }
        EXPECT_EQ(writer.total_bits_written(), 80);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 10);
    for (uint8_t i = 0; i < 10; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

TEST_F(BitFileWriterTest, MoveConstructor)
{
    const fs::path file_path = test_dir / "move_ctor.bin";
    {
        gen::BitFileWriter w1(file_path);
        w1.write_bits(0x42, 8);
        EXPECT_EQ(w1.total_bits_written(), 8);

        gen::BitFileWriter w2(std::move(w1));
        EXPECT_EQ(w2.total_bits_written(), 8);

        w2.write_bits(0x99, 8);
        EXPECT_EQ(w2.total_bits_written(), 16);
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 0x42);
    EXPECT_EQ(data[1], 0x99);
}

TEST_F(BitFileWriterTest, MoveAssignment)
{
    const fs::path file1 = test_dir / "move_assign_1.bin";
    const fs::path file2 = test_dir / "move_assign_2.bin";

    {
        gen::BitFileWriter w1(file1);
        w1.write_bits(0x11, 8);

        gen::BitFileWriter w2(file2);
        w2.write_bits(0x22, 8);

        w1 = std::move(w2);
        // file1 should be closed with 0x11
        // w1 now writes to file2
        w1.write_bits(0x33, 8);
    }

    const auto data1 = read_binary_file(file1);
    ASSERT_EQ(data1.size(), 1);
    EXPECT_EQ(data1[0], 0x11);

    const auto data2 = read_binary_file(file2);
    ASSERT_EQ(data2.size(), 2);
    EXPECT_EQ(data2[0], 0x22);
    EXPECT_EQ(data2[1], 0x33);
}

TEST_F(BitFileWriterTest, ExplicitFlushAndMultipleClose)
{
    const fs::path file_path = test_dir / "flush_close.bin";
    {
        gen::BitFileWriter writer(file_path);
        writer.write_bits(0xAA, 8);
        writer.flush();

        writer.write_bits(0xBB, 8);
        writer.close();
        // Calling close again should be a safe no-op
        writer.close();
    }

    const auto data = read_binary_file(file_path);
    ASSERT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 0xAA);
    EXPECT_EQ(data[1], 0xBB);
}
