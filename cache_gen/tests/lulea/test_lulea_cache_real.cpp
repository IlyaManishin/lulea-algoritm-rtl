#include <gtest/gtest.h>

#include <lulea_verify/lookup_verification.hpp>

#include "lulea_engine/cache_engine_factory.hpp"
#include "lulea_engine/lulea_cache_lookup.hpp"

TEST(LuleaCacheRealTableTest, VerifyCachesWithReference)
{
    tests::CacheEngineFactory<engine::LuleaCacheLookup> factory;
    tests::verify_lookup_engine(factory);
}
