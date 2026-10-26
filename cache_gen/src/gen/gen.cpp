#include "gen/gen.hpp"

namespace gen {

Config::Config(Mode mode, std::string input_path, std::string dest_dir)
    : mode(mode), input_path(std::move(input_path)), dest_dir(std::move(dest_dir)) {}

BuildResult build_lulea_cache(const Config& config) {
    (void)config;
    return BuildResult(CacheStats{});
}

} // namespace gen
