#pragma once

#include "config.hpp"
#include "engine/lookup_engine.hpp"
#include "io/reader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tests
{
    using namespace gen;
    using namespace engine;

    struct LinearMatchResult
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

    // Reference longest prefix match via linear scan over all entries
    class LinearLookup : public ILookupEngine
    {
    public:
        explicit LinearLookup(const MaskEntries &entries);

        // Implements ILookupEngine interface
        port_t lookup(ipv4_t ip) const override;

        // Returns detailed match metadata
        LinearMatchResult match(ipv4_t ip) const noexcept;

        // Parses a human-readable routes.txt file into MaskEntries
        static MaskEntries parse_routes_txt(const std::string &filepath);

        const MaskEntries &entries() const noexcept
        {
            return *this->entries_;
        }

    private:
        const MaskEntries *entries_;
    };

    // Verification algorithm working with any ILookupEngine instances (no templates required)
    LookupVerificationResult verify_lookups(
        const ILookupEngine &target,
        const ILookupEngine &reference,
        const std::vector<ipv4_t> &ips);

} // namespace tests
