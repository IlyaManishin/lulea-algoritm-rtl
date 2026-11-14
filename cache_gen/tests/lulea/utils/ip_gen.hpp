#pragma once

#include "config.hpp"
#include "io/reader.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tests
{
    using namespace gen;

    class IpGen
    {
    public:
        explicit IpGen(const MaskEntries &entries);

        // Generates N random IPv4 addresses using a deterministic PRNG seed
        static std::vector<ipv4_t> ip_gen_random(size_t count, uint32_t seed = 42);

        // Generates sequential addresses starting at 'start' with 'step_size'
        static std::vector<ipv4_t> ip_gen_step(ipv4_t start, ipv4_t step_size = 1, size_t count = 1000);

        // Generates edge cases around all prefixes in the mask list:
        // - Subnet start (base) and second IP
        // - Subnet end (last) and penultimate IP
        // - Center, quarter, 3/4 of the prefix space
        // - One IP before subnet boundary (base - 1)
        // - One IP after subnet boundary (last + 1)
        // - Cross-block boundaries corresponding to L1 (bit 18) and L2 (bit 24)
        std::vector<ipv4_t> ip_gen_edge() const;

        // Convenience aliases
        static std::vector<ipv4_t> random(size_t count, uint32_t seed = 42)
        {
            return ip_gen_random(count, seed);
        }

        static std::vector<ipv4_t> step(ipv4_t start, ipv4_t step_size = 1, size_t count = 1000)
        {
            return ip_gen_step(start, step_size, count);
        }

        std::vector<ipv4_t> edge() const
        {
            return this->ip_gen_edge();
        }

        const MaskEntries &entries() const noexcept
        {
            return *this->entries_;
        }

    private:
        const MaskEntries *entries_;
    };

    using IPGen = IpGen;

} // namespace tests
