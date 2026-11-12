#include "gen/core/lulea_tree.hpp"

#include "gen/config.hpp"
#include "gen/core/cache.hpp"
#include "gen/io/reader.hpp"
#include "gen/lulea_config.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gen
{
    namespace
    {

        struct BitSlice
        {
            size_t shift = 0;
            size_t width = 0;
        };

        struct CellRange
        {
            size_t start = 0;
            size_t count = 0;
        };

        struct PaintValue
        {
            port_t port = NULL_PORT;
            uint8_t prio = 0;
        };

        struct L2CellPos
        {
            size_t block_idx = 0;
            size_t cell_idx = 0;
        };

        struct BuildState
        {
            LuleaTreeConfig cfg;
            LuleaRoute l1;
            std::vector<uint8_t> l1_prio;
            std::vector<LuleaRoute> l2;
            std::vector<std::vector<uint8_t>> l2_prio;
            std::vector<LuleaRoute> l3;
            std::vector<std::vector<uint8_t>> l3_prio;
        };

        void validate_config(const LuleaTreeConfig &cfg)
        {
            if (cfg.l1_bits == 0 || cfg.l2_bits == 0 || cfg.l3_bits == 0)
            {
                throw std::invalid_argument("Lulea level bit widths must be non-zero");
            }
            if ((cfg.l1_bits + cfg.l2_bits + cfg.l3_bits) != IP_SIZE_BITS)
            {
                throw std::invalid_argument("Lulea level bit widths must sum to 32");
            }
        }

        LuleaRoute make_initial_l1(const LuleaTreeConfig &cfg)
        {
            validate_config(cfg);
            return LuleaRoute(size_t{1} << cfg.l1_bits, cfg.l1_cell_bits, NULL_PORT);
        }

        BuildState make_build_state(const LuleaTreeConfig &cfg)
        {
            const size_t l1_size = size_t{1} << cfg.l1_bits;
            return BuildState{
                cfg,
                LuleaRoute(l1_size, cfg.l1_cell_bits, NULL_PORT),
                std::vector<uint8_t>(l1_size, 0),
                {},
                {},
                {},
                {},
            };
        }

        size_t extract_prefix_len(mask_t mask)
        {
            const size_t len = std::to_integer<size_t>(mask);
            if (len > IP_SIZE_BITS)
            {
                throw std::invalid_argument("Prefix length exceeds 32 bits");
            }
            return len;
        }

        ipv4_t prefix_netmask(size_t prefix_len)
        {
            if (prefix_len == 0)
            {
                return 0;
            }
            return static_cast<ipv4_t>(~ipv4_t{0} << (IP_SIZE_BITS - prefix_len));
        }

        size_t level_index(ipv4_t ip_addr, BitSlice slice)
        {
            const ipv4_t mask = (ipv4_t{1} << slice.width) - ipv4_t{1};
            return static_cast<size_t>((ip_addr >> slice.shift) & mask);
        }

        size_t ensure_l2_block(BuildState &state, size_t l1_idx)
        {
            LuleaCell &parent = state.l1[l1_idx];
            if (parent.is_ptr())
            {
                return parent.ptr();
            }

            const size_t block_idx = state.l2.size();
            const size_t block_size = size_t{1} << state.cfg.l2_bits;
            LuleaRoute block(block_size, state.cfg.l2_cell_bits, parent.port());
            // Update parent pointer before appending so overflow leaves state consistent.
            parent.set_ptr(block_idx);

            state.l2.push_back(std::move(block));
            state.l2_prio.emplace_back(block_size, state.l1_prio[l1_idx]);
            return block_idx;
        }

        size_t ensure_l3_block(BuildState &state, L2CellPos pos)
        {
            LuleaCell &parent = state.l2[pos.block_idx][pos.cell_idx];
            if (parent.is_ptr())
            {
                return parent.ptr();
            }

            const size_t block_idx = state.l3.size();
            const size_t block_size = size_t{1} << state.cfg.l3_bits;
            LuleaRoute block(block_size, state.cfg.l3_cell_bits, parent.port());
            parent.set_ptr(block_idx);

            state.l3.push_back(std::move(block));
            state.l3_prio.emplace_back(block_size, state.l2_prio[pos.block_idx][pos.cell_idx]);
            return block_idx;
        }

        void paint_l3_range(BuildState &state, size_t block_idx, CellRange range, PaintValue value)
        {
            LuleaRoute &block = state.l3[block_idx];
            std::vector<uint8_t> &prio = state.l3_prio[block_idx];
            const size_t end_idx = range.start + range.count;

            for (size_t cell_idx = range.start; cell_idx < end_idx; ++cell_idx)
            {
                if (prio[cell_idx] > value.prio)
                {
                    continue;
                }
                block[cell_idx].set_port(value.port);
                prio[cell_idx] = value.prio;
            }
        }

        void paint_l2_range(BuildState &state, size_t block_idx, CellRange range, PaintValue value)
        {
            LuleaRoute &block = state.l2[block_idx];
            std::vector<uint8_t> &prio = state.l2_prio[block_idx];
            const CellRange full_l3{0, size_t{1} << state.cfg.l3_bits};
            const size_t end_idx = range.start + range.count;

            for (size_t cell_idx = range.start; cell_idx < end_idx; ++cell_idx)
            {
                LuleaCell &cell = block[cell_idx];
                if (cell.is_ptr())
                {
                    paint_l3_range(state, cell.ptr(), full_l3, value);
                    continue;
                }
                if (prio[cell_idx] > value.prio)
                {
                    continue;
                }
                cell.set_port(value.port);
                prio[cell_idx] = value.prio;
            }
        }

        void paint_l1_range(BuildState &state, CellRange range, PaintValue value)
        {
            const CellRange full_l2{0, size_t{1} << state.cfg.l2_bits};
            const size_t end_idx = range.start + range.count;

            for (size_t cell_idx = range.start; cell_idx < end_idx; ++cell_idx)
            {
                LuleaCell &cell = state.l1[cell_idx];
                if (cell.is_ptr())
                {
                    paint_l2_range(state, cell.ptr(), full_l2, value);
                    continue;
                }
                if (state.l1_prio[cell_idx] > value.prio)
                {
                    continue;
                }
                cell.set_port(value.port);
                state.l1_prio[cell_idx] = value.prio;
            }
        }

        void insert_entry(BuildState &state, const TMaskEntry &entry)
        {
            const size_t prefix_len = extract_prefix_len(entry.mask);
            const ipv4_t masked_ip = entry.ip & prefix_netmask(prefix_len);
            const PaintValue value{entry.port, static_cast<uint8_t>(prefix_len)};

            const size_t end1 = state.cfg.l1_bits;
            const size_t end2 = end1 + state.cfg.l2_bits;
            const size_t l1_shift = state.cfg.l2_bits + state.cfg.l3_bits;

            if (prefix_len <= end1)
            {
                const size_t start = level_index(masked_ip, {l1_shift, state.cfg.l1_bits});
                const size_t count = size_t{1} << (end1 - prefix_len);
                paint_l1_range(state, {start, count}, value);
                return;
            }

            const size_t l1_idx = level_index(masked_ip, {l1_shift, state.cfg.l1_bits});
            const size_t l2_block_idx = ensure_l2_block(state, l1_idx);

            if (prefix_len <= end2)
            {
                const size_t start = level_index(masked_ip, {state.cfg.l3_bits, state.cfg.l2_bits});
                const size_t count = size_t{1} << (end2 - prefix_len);
                paint_l2_range(state, l2_block_idx, {start, count}, value);
                return;
            }

            const size_t l2_idx = level_index(masked_ip, {state.cfg.l3_bits, state.cfg.l2_bits});
            const size_t l3_block_idx = ensure_l3_block(state, {l2_block_idx, l2_idx});
            const size_t start = level_index(masked_ip, {0, state.cfg.l3_bits});
            const size_t count = size_t{1} << (IP_SIZE_BITS - prefix_len);
            paint_l3_range(state, l3_block_idx, {start, count}, value);
        }

    } // namespace

    LuleaTree::LuleaTree(const LuleaTreeConfig &tree_config)
        : config_(tree_config), l1_(make_initial_l1(tree_config))
    {
    }

    LuleaTree::LuleaTree(const MaskEntries &entries, const LuleaTreeConfig &tree_config)
        : LuleaTree(tree_config)
    {
        this->build(entries);
    }

    void LuleaTree::build(const MaskEntries &entries)
    {
        validate_config(this->config_);
        BuildState state = make_build_state(this->config_);

        for (const TMaskEntry &entry : entries)
        {
            insert_entry(state, entry);
        }

        this->l1_ = std::move(state.l1);
        this->l2_ = std::move(state.l2);
        this->l3_ = std::move(state.l3);
    }

    port_t LuleaTree::lookup(ipv4_t ip_addr) const
    {
        const size_t l1_shift = this->config_.l2_bits + this->config_.l3_bits;
        const size_t l1_idx = level_index(ip_addr, {l1_shift, this->config_.l1_bits});
        const LuleaCell &cell1 = this->l1_[l1_idx];
        if (cell1.is_port())
        {
            return cell1.port();
        }

        const size_t l2_idx = level_index(ip_addr, {this->config_.l3_bits, this->config_.l2_bits});
        const LuleaCell &cell2 = this->l2_[cell1.ptr()][l2_idx];
        if (cell2.is_port())
        {
            return cell2.port();
        }

        const size_t l3_idx = level_index(ip_addr, {0, this->config_.l3_bits});
        return this->l3_[cell2.ptr()][l3_idx].port();
    }

} // namespace gen
