#pragma once

#include "config.hpp"
#include "io/reader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gen
{

    struct LpmMatch
    {
        bool matched = false;
        port_t port = NULL_PORT;
        mask_t mask = mask_t{0};
        ipv4_t prefix = 0;
    };

    struct LookupVerificationResult
    {
        size_t total = 0;
        size_t matched = 0;
        size_t mismatched = 0;
        std::vector<ipv4_t> mismatch_ips;

        bool is_success() const noexcept
        {
            return this->mismatched == 0 && this->total > 0;
        }
    };

    class ReferenceLpm
    {
    public:
        explicit ReferenceLpm(const MaskEntries &entries);

        // Universal lookup interface matching LuleaTree / Cache lookup
        port_t lookup(ipv4_t ip) const noexcept;

        // Detailed match info
        LpmMatch match(ipv4_t ip) const noexcept;

        // Parses a human-readable routes.txt file into MaskEntries
        static MaskEntries parse_routes_txt(const std::string &filepath);

        const MaskEntries &entries() const noexcept
        {
            return this->entries_;
        }

    private:
        const MaskEntries &entries_;
    };

    // Generic verification algorithm: compares any lookup engine against ReferenceLpm
    template <typename LookupEngine>
    LookupVerificationResult verify_lookups(
        const LookupEngine &engine,
        const ReferenceLpm &reference,
        const std::vector<ipv4_t> &ips)
    {
        LookupVerificationResult result;
        result.total = ips.size();

        for (const ipv4_t ip : ips)
        {
            const port_t actual = engine.lookup(ip);
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

} // namespace gen
