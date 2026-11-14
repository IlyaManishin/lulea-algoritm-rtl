#include <gtest/gtest.h>

#include "config.hpp"
#include "io/reader.hpp"
#include "utils/linear_lookup.hpp"

TEST(LinearLookupTest, MatchesLongestPrefixAndHandlesDefaultRoute)
{
    const gen::MaskEntries entries = {
        gen::TMaskEntry{.ip = 0x00000000U, .mask = gen::mask_t{0}, .port = gen::port_t{10}},
        gen::TMaskEntry{.ip = 0x0A000000U, .mask = gen::mask_t{8}, .port = gen::port_t{1}},
        gen::TMaskEntry{.ip = 0x0A010000U, .mask = gen::mask_t{16}, .port = gen::port_t{2}},
        gen::TMaskEntry{.ip = 0x0A010200U, .mask = gen::mask_t{24}, .port = gen::port_t{3}},
    };

    const tests::LinearLookup linear(entries);

    // /24 match
    EXPECT_EQ(linear.lookup(0x0A010205U), gen::port_t{3});
    // /16 match
    EXPECT_EQ(linear.lookup(0x0A010305U), gen::port_t{2});
    // /8 match
    EXPECT_EQ(linear.lookup(0x0A050505U), gen::port_t{1});
    // default route match
    EXPECT_EQ(linear.lookup(0xC0A80101U), gen::port_t{10});
}
