#include "./gen.hpp"
#include "config.hpp"

#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

const size_t BUFFER_ENTRIES_COUNT = 4096;
const size_t BUFFER_SIZE = ENTRY_SIZE * BUFFER_ENTRIES_COUNT;

typedef struct TMaskEntry
{
    ipv4_t ip;
    mask_t mask;
    port_t port;
} TMaskEntry;

using MaskEntries = std::vector<TMaskEntry>;

namespace gen
{

    static MaskEntries *read_mask_entries(std::ifstream &file, bin_entry_count_t count)
    {
        MaskEntries *entries = new MaskEntries(count);
        std::vector<char> buf(BUFFER_SIZE);

        size_t processed = 0;
        while (processed < count)
        {
            size_t batch = std::min(static_cast<size_t>(count - processed), BUFFER_ENTRIES_COUNT);

            if (!file.read(buf.data(), batch * ENTRY_SIZE))
            {
                delete entries;
                throw std::runtime_error("Unexpected EOF while reading mask entries");
            }

            for (size_t i = 0; i < batch; ++i)
            {
                TMaskEntry &entry = (*entries)[processed + i];
                const char *entry_buf = buf.data() + i * ENTRY_SIZE;
                size_t off = 0;

                std::memcpy(&entry.ip, entry_buf + off, sizeof(ipv4_t));
                off += sizeof(ipv4_t);

                std::memcpy(&entry.mask, entry_buf + off, sizeof(mask_t));
                off += sizeof(mask_t);

                std::memcpy(&entry.port, entry_buf + off, sizeof(port_t));
            }

            processed += batch;
        }

        return entries;
    }

    static MaskEntries *read_mask_mapping(const std::string &map_path)
    {
        std::ifstream map_file(map_path, std::ios::binary);
        if (!map_file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + map_path);
        }

        std::vector<std::byte> header(HEADER_SIZE);
        map_file.read(reinterpret_cast<char *>(header.data()), HEADER_SIZE);

        if (map_file.gcount() != (std::streamsize)HEADER_SIZE)
        {
            throw std::runtime_error("Unexpected EOF while reading header in: " + map_path);
        }

        if (std::memcmp(header.data(), BIN_MAGIC.data(), BIN_MAGIC.size()) != 0)
        {
            throw std::runtime_error("Invalid magic signature in: " + map_path);
        }

        bin_entry_count_t mask_count;
        std::memcpy(&mask_count, header.data() + BIN_MAGIC.size(), sizeof(bin_entry_count_t));

        return read_mask_entries(map_file, mask_count);
    }

BuildResult build_lulea_cache(const Config &config)
{
    (void)config;
    return BuildResult{.success = true, .error_message = "", .stats = CacheStats{}};
}

} // namespace gen
