#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

namespace gen
{

    class BitFileWriter
    {
    public:
        explicit BitFileWriter(const std::filesystem::path &file_path);
        explicit BitFileWriter(const std::filesystem::path &file_path, size_t buffer_size);
        ~BitFileWriter();

        BitFileWriter(const BitFileWriter &) = delete;
        BitFileWriter &operator=(const BitFileWriter &) = delete;

        BitFileWriter(BitFileWriter &&other) noexcept;
        BitFileWriter &operator=(BitFileWriter &&other) noexcept;

        void write_bits(uint64_t value, size_t num_bits);
        void write_bytes(std::span<const std::byte> bytes, size_t num_bits);
        void write_bytes(std::span<const std::byte> bytes);

        void flush();
        void close();

        size_t total_bits_written() const noexcept;

    private:
        void flush_buffer_to_file();

        std::ofstream file_;
        std::vector<std::byte> byte_buffer_;
        size_t buffer_capacity_;

        uint64_t accumulator_{0};
        size_t accumulated_bits_{0};
        size_t total_bits_{0};
    };

} // namespace gen