#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace gen
{

    class GenException : public std::runtime_error
    {
    public:
        explicit GenException(const std::string &message)
            : std::runtime_error(message) {}

        explicit GenException(const char *message)
            : std::runtime_error(message) {}
    };

    class FatalGenException : public GenException
    {
    public:
        explicit FatalGenException(const std::string &message)
            : GenException(message) {}

        explicit FatalGenException(const char *message)
            : GenException(message) {}
    };

    enum class Mode : std::uint8_t
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

    struct CacheByteSizes
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

    struct CacheStats
    {
        CacheByteSizes cache_sizes;
        CacheByteSizes cache_sizes_aligned{};
    };

    struct BuildResult
    {
        CacheStats stats;
    };

    BuildResult build_lulea_cache(const Config &config);

} // namespace gen
