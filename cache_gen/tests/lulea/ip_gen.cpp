#include "ip_gen.hpp"

#include "config.hpp"
#include "io/reader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace gen
{

    IpGen::IpGen(const MaskEntries &entries)
        : entries_(entries)
    {
    }

    std::vector<ipv4_t> IpGen::ip_gen_random(size_t count, uint32_t seed)
    {
        std::vector<ipv4_t> result(count);
        std::mt19937 rng(seed);
        std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

        for (size_t i = 0; i < count; ++i)
        {
            result[i] = dist(rng);
        }

        return result;
    }

    std::vector<ipv4_t> IpGen::ip_gen_step(ipv4_t start, ipv4_t step_size, size_t count)
    {
        std::vector<ipv4_t> result;
        result.reserve(count);

        ipv4_t current = start;
        for (size_t i = 0; i < count; ++i)
        {
            result.push_back(current);
            current += step_size;
        }

        return result;
    }

    std::vector<ipv4_t> IpGen::ip_gen_edge() const
    {
        std::vector<ipv4_t> result;
        result.reserve(this->entries_.size() * 10);

        for (const auto &entry : this->entries_)
        {
            const size_t len = static_cast<size_t>(std::to_integer<uint8_t>(entry.mask));
            if (len > 32)
            {
                continue;
            }

            const ipv4_t netmask = (len == 0) ? 0u : static_cast<ipv4_t>(~ipv4_t{0} << (32 - len));
            const ipv4_t base = entry.ip & netmask;
            const uint64_t size = (len == 32) ? 1ULL : (1ULL << (32 - len));
            const ipv4_t last = static_cast<ipv4_t>(base + size - 1ULL);

            // 1. Inside subnet boundaries
            result.push_back(base);
            if (size > 1ULL)
            {
                result.push_back(base + 1u);
                result.push_back(last);
            }
            if (size > 2ULL)
            {
                result.push_back(last - 1u);
            }
            if (size > 3ULL)
            {
                result.push_back(static_cast<ipv4_t>(base + (size / 2ULL)));
                result.push_back(static_cast<ipv4_t>(base + (size / 4ULL)));
                result.push_back(static_cast<ipv4_t>(base + ((3ULL * size) / 4ULL)));
            }

            // 2. Just outside subnet boundaries
            if (base > 0u)
            {
                result.push_back(base - 1u);
            }
            if (last < 0xFFFFFFFFu)
            {
                result.push_back(last + 1u);
            }

            // 3. Level boundaries: L1 boundary (18 bits -> remaining 14 bits mask)
            constexpr ipv4_t L1_SLICE_MASK = 0x00003FFFu;
            const ipv4_t l1_boundary_base = base & ~L1_SLICE_MASK;
            result.push_back(l1_boundary_base);
            result.push_back(l1_boundary_base + L1_SLICE_MASK);
            if (l1_boundary_base > 0u)
            {
                result.push_back(l1_boundary_base - 1u);
            }
            if (static_cast<uint64_t>(l1_boundary_base) + L1_SLICE_MASK < 0xFFFFFFFFu)
            {
                result.push_back(l1_boundary_base + L1_SLICE_MASK + 1u);
            }

            // 4. Level boundaries: L2 boundary (8 bits remaining)
            constexpr ipv4_t L2_SLICE_MASK = 0x000000FFu;
            const ipv4_t l2_boundary_base = base & ~L2_SLICE_MASK;
            result.push_back(l2_boundary_base);
            result.push_back(l2_boundary_base + L2_SLICE_MASK);
            if (l2_boundary_base > 0u)
            {
                result.push_back(l2_boundary_base - 1u);
            }
            if (static_cast<uint64_t>(l2_boundary_base) + L2_SLICE_MASK < 0xFFFFFFFFu)
            {
                result.push_back(l2_boundary_base + L2_SLICE_MASK + 1u);
            }
        }

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());

        return result;
    }

} // namespace gen
