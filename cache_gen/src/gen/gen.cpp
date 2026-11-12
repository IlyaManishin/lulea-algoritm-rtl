#include "gen.hpp"

namespace gen
{

    BuildResult build_lulea_cache(const Config &config)
    {
        (void)config;
        // catch lulea algo exceptions here
        return BuildResult{.success = true, .error_message = "", .stats = CacheStats{}};
    }

} // namespace gen
