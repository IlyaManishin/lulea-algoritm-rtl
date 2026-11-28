#pragma once

#include "lulea_config.hpp"
#include "lulea_data/route.hpp"
#include <lulea_common/config.hpp>
#include <lulea_common/reader.hpp>

#include <cstddef>
#include <vector>

namespace gen
{

    struct LuleaTreeConfig
    {
        size_t l1_bits = L1_IP_SIZE;
        size_t l2_bits = L2_IP_SIZE;
        size_t l3_bits = L3_IP_SIZE;
        size_t l1_cell_bits = L1_DEFAULT_SEQ_CELL;
        size_t l2_cell_bits = L2_DEFAULT_SEQ_CELL;
        size_t l3_cell_bits = L3_DEFAULT_SEQ_CELL;
    };

    class LuleaTree
    {
    public:
        explicit LuleaTree(const LuleaTreeConfig &tree_config = {});
        explicit LuleaTree(const MaskEntries &entries, const LuleaTreeConfig &tree_config = {});

        void build(const MaskEntries &entries);

        bool is_built() const noexcept { return this->built_; }
        const LuleaTreeConfig &config() const noexcept { return this->config_; }
        const LuleaRoute &l1() const noexcept { return this->l1_; }
        const std::vector<LuleaRoute> &l2() const noexcept { return this->l2_; }
        const std::vector<LuleaRoute> &l3() const noexcept { return this->l3_; }

    private:
        LuleaTreeConfig config_;
        LuleaRoute l1_;
        std::vector<LuleaRoute> l2_;
        std::vector<LuleaRoute> l3_;
        bool built_ = false;
    };

} // namespace gen
