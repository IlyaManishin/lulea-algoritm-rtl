#include "bit_file_writer.hpp"
#include <algorithm>
#include <stdexcept>

namespace gen
{

    BitFileWriter::BitFileWriter(const std::filesystem::path &file_path, std::size_t buffer_size)
        : buffer_capacity_(buffer_size)
    {
        file_.open(file_path, std::ios::binary);
        if (!file_.is_open())
        {
            throw std::runtime_error("Failed to open file: " + file_path.string());
        }
        byte_buffer_.reserve(buffer_capacity_);
    }

    BitFileWriter::~BitFileWriter()
    {
        try
        {
            close();
        }
        catch (...)
        {
        }
    }

    BitFileWriter::BitFileWriter(BitFileWriter &&other) noexcept
        : file_(std::move(other.file_)),
          byte_buffer_(std::move(other.byte_buffer_)),
          buffer_capacity_(other.buffer_capacity_),
          accumulator_(other.accumulator_),
          accumulated_bits_(other.accumulated_bits_),
          total_bits_(other.total_bits_)
    {
        other.buffer_capacity_ = 0;
        other.accumulator_ = 0;
        other.accumulated_bits_ = 0;
        other.total_bits_ = 0;
    }

    BitFileWriter &BitFileWriter::operator=(BitFileWriter &&other) noexcept
    {
        if (this != &other)
        {
            try
            {
                close();
            }
            catch (...)
            {
            }

            file_ = std::move(other.file_);
            byte_buffer_ = std::move(other.byte_buffer_);
            buffer_capacity_ = other.buffer_capacity_;
            accumulator_ = other.accumulator_;
            accumulated_bits_ = other.accumulated_bits_;
            total_bits_ = other.total_bits_;

            other.buffer_capacity_ = 0;
            other.accumulator_ = 0;
            other.accumulated_bits_ = 0;
            other.total_bits_ = 0;
        }
        return *this;
    }

    void BitFileWriter::write_bits(uint64_t value, std::size_t num_bits)
    {
        if (num_bits == 0)
        {
            return;
        }

        total_bits_ += num_bits;

        while (num_bits > 0)
        {
            const std::size_t space_in_accumulator = 64 - accumulated_bits_;
            const std::size_t bits_to_take = std::min(num_bits, space_in_accumulator);

            const uint64_t mask = (bits_to_take == 64) ? ~0ULL : ((1ULL << bits_to_take) - 1);
            const uint64_t chunk = value & mask;

            accumulator_ |= (chunk << accumulated_bits_);
            accumulated_bits_ += bits_to_take;

            if (bits_to_take == 64)
            {
                value = 0;
            }
            else
            {
                value >>= bits_to_take;
            }
            num_bits -= bits_to_take;

            while (accumulated_bits_ >= 8)
            {
                byte_buffer_.push_back(static_cast<std::byte>(accumulator_ & 0xFF));
                accumulator_ >>= 8;
                accumulated_bits_ -= 8;

                if (byte_buffer_.size() >= buffer_capacity_)
                {
                    flush_buffer_to_file();
                }
            }
        }
    }

    void BitFileWriter::write_bytes(std::span<const std::byte> bytes, std::size_t num_bits)
    {
        std::size_t remaining_bits = std::min(num_bits, bytes.size() * 8);

        for (std::byte b : bytes)
        {
            if (remaining_bits == 0)
            {
                break;
            }
            const std::size_t bits_to_write = std::min<std::size_t>(remaining_bits, 8);
            write_bits(static_cast<uint64_t>(std::to_integer<uint8_t>(b)), bits_to_write);
            remaining_bits -= bits_to_write;
        }
    }

    void BitFileWriter::write_bytes(std::span<const std::byte> bytes)
    {
        write_bytes(bytes, bytes.size() * 8);
    }

    void BitFileWriter::flush_buffer_to_file()
    {
        if (byte_buffer_.empty())
        {
            return;
        }

        if (!file_.write(reinterpret_cast<const char *>(byte_buffer_.data()),
                         static_cast<std::streamsize>(byte_buffer_.size())))
        {
            throw std::runtime_error("I/O error: failed to write buffer to output file");
        }

        byte_buffer_.clear();
    }

    void BitFileWriter::flush()
    {
        flush_buffer_to_file();
        file_.flush();
    }

    void BitFileWriter::close()
    {
        if (!file_.is_open())
        {
            return;
        }

        if (accumulated_bits_ > 0)
        {
            byte_buffer_.push_back(static_cast<std::byte>(accumulator_ & 0xFF));
            accumulator_ = 0;
            accumulated_bits_ = 0;
        }

        flush_buffer_to_file();
        file_.close();
    }

    std::size_t BitFileWriter::total_bits_written() const noexcept
    {
        return total_bits_;
    }

} // namespace gen