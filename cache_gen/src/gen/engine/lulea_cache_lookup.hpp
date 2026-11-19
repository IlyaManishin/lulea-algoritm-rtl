#pragma once

#include "config.hpp"
#include "engine/lulea_tree_lookup.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace engine
{
    struct CacheLevelBuffers
    {
        std::vector<uint8_t> pop;
        std::vector<uint8_t> chunk;
        std::vector<uint8_t> seq;
    };

    struct LevelMemory
    {
        std::vector<uint64_t> pop;
        std::vector<uint32_t> chunk;
        std::vector<uint32_t> seq;
    };

    class LuleaCacheLookup : public ILookupEngine
    {
    public:
        explicit LuleaCacheLookup(const std::filesystem::path &cache_dir);
        LuleaCacheLookup(CacheLevelBuffers l1, CacheLevelBuffers l2, CacheLevelBuffers l3);

        port_t lookup(ipv4_t ip) const override;

        const CacheLevelBuffers &l1() const noexcept { return this->l1_; }
        const CacheLevelBuffers &l2() const noexcept { return this->l2_; }
        const CacheLevelBuffers &l3() const noexcept { return this->l3_; }

    private:
        CacheLevelBuffers l1_;
        CacheLevelBuffers l2_;
        CacheLevelBuffers l3_;

        LevelMemory l1_mem_;
        LevelMemory l2_mem_;
        LevelMemory l3_mem_;
    };

} // namespace engine