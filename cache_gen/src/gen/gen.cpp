#include "./gen.hpp"
#include "config.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

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
        char buf[ENTRY_SIZE];

        for (size_t i = 0; i < count; i++)
        {
            if (!file.read(buf, ENTRY_SIZE))
            {
                delete entries;
                // throw
            }
            TMaskEntry &entry = (*entries)[i];
            size_t off = 0;

            memcpy(&entry.ip, buf, sizeof(ipv4_t));
            off += sizeof(ipv4_t);

            memcpy(&entry.mask, buf + off, sizeof(mask_t));
            off += sizeof(mask_t);

            memcpy(&entry.port, buf + off, sizeof(port_t));
        }
        return entries;
    }

    static MaskEntries *read_mask_mapping(const std::string &map_path)
    {
        std::ifstream map_file(map_path, std::ios::binary);
        if (!map_file.is_open())
        {
            throw std::runtime_error("");
        }

        std::vector<std::byte> header(HEADER_SIZE);
        map_file.read(reinterpret_cast<char *>(header.data()), HEADER_SIZE);
        if (map_file.gcount() != HEADER_SIZE || std::memcmp(header.data(), BIN_MAGIC.data(), BIN_MAGIC.size()) != 0)
        {
            throw std::runtime_error("");
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
