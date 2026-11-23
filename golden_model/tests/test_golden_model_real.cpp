#include <gtest/gtest.h>

#include <lulea_verify/lookup_verification.hpp>

#include "cache_engine_factory.hpp"
#include "golden_model_lookup.hpp"

TEST(GoldenModelRealTableTest, VerifyCachesWithReference)
{
    tests::CacheEngineFactory<tests::GoldenModelLookup> factory;
    tests::verify_lookup_engine(factory);
}
