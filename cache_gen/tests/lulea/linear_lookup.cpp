#include "linear_lookup.hpp"

#include "config.hpp"
#include "engine/lookup_engine.hpp"
#include "io/reader.hpp"

#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tests
{

    LinearLookup::LinearLookup(const MaskEntries &entries)
        : entries_(&entries)
    {
    }

    port_t LinearLookup::lookup(ipv4_t ip) const
    {
        return this->match(ip).port;
    }

    LinearMatchResult LinearLookup::match(ipv4_t ip) const noexcept
    {
        LinearMatchResult best;
        int best_len = -1;

        for (const auto &entry : *this->entries_)
        {
            const int len = std::to_integer<int>(entry.mask);
            if (len < 0 || len > 32)
            {
                continue;
            }

            const ipv4_t netmask = (len == 0) ? 0u : static_cast<ipv4_t>(~ipv4_t{0} << (32 - len));
            if ((ip & netmask) == (entry.ip & netmask))
            {
                // Later entries of the same prefix length overwrite earlier ones (exact LPM match order)
                if (len >= best_len)
                {
                    best_len = len;
                    best.matched = true;
                    best.port = entry.port;
                    best.mask = entry.mask;
                    best.prefix = entry.ip & netmask;
                }
            }
        }

        return best;
    }

    MaskEntries LinearLookup::parse_routes_txt(const std::string &filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open text routes file: " + filepath);
        }

        std::string line;
        if (!std::getline(file, line))
        {
            throw std::runtime_error("Empty text routes file: " + filepath);
        }

        size_t expected_count = 0;
        try
        {
            expected_count = std::stoull(line);
        }
        catch (const std::exception &)
        {
            throw std::runtime_error("Invalid header count in: " + filepath + " (" + line + ")");
        }

        MaskEntries entries;
        entries.reserve(expected_count);

        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            std::stringstream ss(line);
            std::string prefix_str;
            unsigned int raw_port = 0;

            if (!(ss >> prefix_str >> raw_port))
            {
                continue;
            }

            const size_t slash_pos = prefix_str.find('/');
            if (slash_pos == std::string::npos)
            {
                throw std::runtime_error("Malformed prefix missing slash: " + prefix_str);
            }

            const std::string ip_part = prefix_str.substr(0, slash_pos);
            const std::string mask_part = prefix_str.substr(slash_pos + 1);

            struct in_addr addr
            {
            };
            if (::inet_pton(AF_INET, ip_part.c_str(), &addr) != 1)
            {
                throw std::runtime_error("Invalid IPv4 address: " + ip_part);
            }

            const ipv4_t ip = ntohl(addr.s_addr);
            const unsigned long mask_len = std::stoul(mask_part);
            if (mask_len > 32)
            {
                throw std::runtime_error("Invalid mask length: " + mask_part);
            }
            if (raw_port > 255)
            {
                throw PortOverflowError("Port ID exceeds 255 in routes.txt: " + std::to_string(raw_port));
            }

            TMaskEntry entry;
            entry.ip = ip;
            entry.mask = static_cast<mask_t>(static_cast<uint8_t>(mask_len));
            entry.port = static_cast<port_t>(static_cast<uint8_t>(raw_port));
            entries.push_back(entry);
        }

        return entries;
    }

    LookupVerificationResult verify_lookups(
        const ILookupEngine &target,
        const ILookupEngine &reference,
        const std::vector<ipv4_t> &ips)
    {
        LookupVerificationResult result;
        result.total = ips.size();

        for (const ipv4_t ip : ips)
        {
            const port_t actual = target.lookup(ip);
            const port_t expected = reference.lookup(ip);

            if (actual == expected)
            {
                result.matched++;
            }
            else
            {
                result.mismatched++;
                if (result.mismatch_ips.size() < 16)
                {
                    result.mismatch_ips.push_back(ip);
                }
            }
        }

        return result;
    }

} // namespace tests
