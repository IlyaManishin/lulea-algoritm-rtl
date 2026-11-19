#pragma once

#include <cstdint>
#include <string>

#include "types.hpp"

namespace gen
{

    enum class Mode
    {
        Stats,
        Dump
    };

    struct Config
    {
        Mode mode = Mode::Stats;
        std::string input_path;
        std::string dest_dir;
    };

    struct CacheStats
    {
        uint32_t l1_pop = 0;
        uint32_t l1_chunk = 0;
        uint32_t l1_seq = 0;

        uint32_t l2_pop = 0;
        uint32_t l2_chunk = 0;
        uint32_t l2_seq = 0;

        uint32_t l3_pop = 0;
        uint32_t l3_chunk = 0;
        uint32_t l3_seq = 0;
    };

    struct BuildResult
    {
        bool success = false;
        std::string error_message;
        CacheStats stats;
    };

    BuildResult build_lulea_cache(const Config &config);

} // namespace gen
