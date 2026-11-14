#include "engine/lulea_tree_lookup.hpp"

#include "config.hpp"
#include "core/cache.hpp"
#include "core/lulea_tree.hpp"

#include <cstddef>

namespace engine
{
    namespace
    {

        // Extracts a bit slice of length 'width' at bit offset 'shift' from the IPv4 address
        inline size_t extract_slice(ipv4_t ip, size_t shift, size_t width) noexcept
        {
            const ipv4_t shifted_ip = ip >> shift;

            ipv4_t mask = ~ipv4_t{0};
            if (width < 32)
            {
                mask = static_cast<ipv4_t>((ipv4_t{1U} << width) - 1U);
            }

            const ipv4_t slice = shifted_ip & mask;
            return static_cast<size_t>(slice);
        }

    } // namespace

    LuleaTreeLookup::LuleaTreeLookup(const LuleaTree &tree)
        : tree_(&tree) {};

    port_t LuleaTreeLookup::lookup(ipv4_t ip) const
    {
        const LuleaTreeConfig &cfg = this->tree_->config();

        // Level 1: inspect root routing table
        const size_t l1_shift = cfg.l2_bits + cfg.l3_bits;
        const size_t l1_idx = extract_slice(ip, l1_shift, cfg.l1_bits);
        const LuleaCell &l1_cell = this->tree_->l1()[l1_idx];
        if (l1_cell.is_port())
        {
            return l1_cell.port();
        }

        // Level 2: follow pointer into L2 block
        const size_t l2_shift = cfg.l3_bits;
        const size_t l2_idx = extract_slice(ip, l2_shift, cfg.l2_bits);
        const size_t l2_block_idx = l1_cell.ptr();
        const LuleaCell &l2_cell = this->tree_->l2()[l2_block_idx][l2_idx];
        if (l2_cell.is_port())
        {
            return l2_cell.port();
        }

        // Level 3: follow pointer into leaf L3 block
        const size_t l3_idx = extract_slice(ip, 0, cfg.l3_bits);
        const size_t l3_block_idx = l2_cell.ptr();
        const LuleaCell &l3_cell = this->tree_->l3()[l3_block_idx][l3_idx];
        return l3_cell.port();
    }

} // namespace engine
