#include "reader.hpp"

#include "config.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

namespace gen
{
    namespace
    {
        const size_t BUFFER_ENTRIES_COUNT = 4096;
        const size_t BUFFER_SIZE = ENTRY_SIZE * BUFFER_ENTRIES_COUNT;

        port_t truncate_port(reader_port_t raw_port)
        {
            if constexpr (sizeof(port_t) < sizeof(reader_port_t))
            {
                if ((raw_port >> (sizeof(port_t) * 8)) != 0)
                {
                    throw PortOverflowError("Port ID " + std::to_string(raw_port) +
                                            " exceeds port_t capacity");
                }
            }
            return static_cast<port_t>(raw_port);
        }

        MaskEntries read_mask_entries(std::ifstream &file, bin_entry_count_t count)
        {
            MaskEntries entries(count);
            std::vector<char> buf(BUFFER_SIZE);

            size_t processed = 0;
            while (processed < count)
            {
                const size_t batch = std::min(static_cast<size_t>(count - processed), BUFFER_ENTRIES_COUNT);
                const size_t read_bytes = batch * ENTRY_SIZE;

                if (!file.read(buf.data(), static_cast<std::streamsize>(read_bytes)))
                {
                    throw std::runtime_error("Unexpected EOF while reading mask entries");
                }

                for (size_t i = 0; i < batch; ++i)
                {
                    TMaskEntry &entry = entries[processed + i];
                    const char *entry_buf = buf.data() + i * ENTRY_SIZE;
                    size_t off = 0;

                    std::memcpy(&entry.ip, entry_buf + off, sizeof(ipv4_t));
                    off += sizeof(ipv4_t);

                    std::memcpy(&entry.mask, entry_buf + off, sizeof(mask_t));
                    off += sizeof(mask_t);

                    reader_port_t raw_port = 0;
                    std::memcpy(&raw_port, entry_buf + off, sizeof(reader_port_t));
                    entry.port = truncate_port(raw_port);
                }

                processed += batch;
            }

            return entries;
        }
    } // namespace

    Reader::Reader(const std::string &filepath)
        : filepath_(filepath) {}

    MaskEntries Reader::read()
    {
        std::ifstream map_file(this->filepath_, std::ios::binary);
        if (!map_file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + this->filepath_);
        }

        std::vector<std::byte> header(HEADER_SIZE);
        map_file.read(reinterpret_cast<char *>(header.data()), HEADER_SIZE);

        if (map_file.gcount() != static_cast<std::streamsize>(HEADER_SIZE))
        {
            throw std::runtime_error("Unexpected EOF while reading header in: " + this->filepath_);
        }

        if (std::memcmp(header.data(), BIN_MAGIC.data(), BIN_MAGIC.size()) != 0)
        {
            throw std::runtime_error("Invalid magic signature in: " + this->filepath_);
        }

        bin_entry_count_t mask_count = 0;
        std::memcpy(&mask_count, header.data() + BIN_MAGIC.size(), sizeof(bin_entry_count_t));

        return read_mask_entries(map_file, mask_count);
    }

    MaskEntries Reader::read(const std::string &filepath)
    {
        Reader reader(filepath);
        return reader.read();
    }

} // namespace gen
