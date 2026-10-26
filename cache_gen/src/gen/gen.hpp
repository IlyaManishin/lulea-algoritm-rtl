#pragma once

#include <cstdint>
#include <string>

#include "gen/config.hpp"

namespace gen {

struct CacheStats {
    uint32_t l1_pop   = 0;
    uint32_t l1_chunk = 0;
    uint32_t l1_seq   = 0;

    uint32_t l2_pop   = 0;
    uint32_t l2_chunk = 0;
    uint32_t l2_seq   = 0;

    uint32_t l3_pop   = 0;
    uint32_t l3_chunk = 0;
    uint32_t l3_seq   = 0;
};

class BuildResult {
public:
    BuildResult() = default;
    explicit BuildResult(CacheStats stats) : stats(stats), success(true) {}
    BuildResult(bool success, std::string error_message)
        : stats(), success(success), error_message(std::move(error_message)) {}

    bool is_success() const { return this->success; }
    const CacheStats& get_stats() const { return this->stats; }
    const std::string& get_error_message() const { return this->error_message; }

private:
    CacheStats stats;
    bool success = false;
    std::string error_message;
};

// Generator library interface
BuildResult build_lulea_cache(const Config& config);

} // namespace gen
