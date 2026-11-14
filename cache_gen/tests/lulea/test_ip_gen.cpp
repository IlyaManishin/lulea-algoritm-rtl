#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "config.hpp"
#include "io/reader.hpp"
#include "utils/ip_gen.hpp"

TEST(IpGenTest, GeneratesDeterministicRandomIps)
{
    const gen::MaskEntries entries = {
        gen::TMaskEntry{.ip = 0x0A000000U, .mask = gen::mask_t{8}, .port = gen::port_t{1}},
    };

    const tests::IpGen ip_gen(entries);
    const auto ips1 = ip_gen.ip_gen_random(100, 42);
    const auto ips2 = ip_gen.ip_gen_random(100, 42);
    const auto ips_diff = ip_gen.ip_gen_random(100, 43);

    EXPECT_EQ(ips1.size(), 100U);
    EXPECT_EQ(ips1, ips2);
    EXPECT_NE(ips1, ips_diff);
}

TEST(IpGenTest, GeneratesStepProgression)
{
    const gen::MaskEntries entries;
    const tests::IpGen ip_gen(entries);

    const auto ips = ip_gen.ip_gen_step(10, 5, 4);
    const std::vector<gen::ipv4_t> expected = {10, 15, 20, 25};
    EXPECT_EQ(ips, expected);
}

TEST(IpGenTest, GeneratesSubnetEdgeCasesAndBoundaries)
{
    const gen::MaskEntries entries = {
        gen::TMaskEntry{.ip = 0xC0A80100U, .mask = gen::mask_t{24}, .port = gen::port_t{1}}, // 192.168.1.0/24
    };

    const tests::IpGen ip_gen(entries);
    const auto edge_ips = ip_gen.ip_gen_edge();

    EXPECT_FALSE(edge_ips.empty());

    // Must contain base, last, center, base-1, last+1
    auto contains_ip = [&edge_ips](gen::ipv4_t target) {
        return std::binary_search(edge_ips.begin(), edge_ips.end(), target);
    };

    EXPECT_TRUE(contains_ip(0xC0A80100U));        // base
    EXPECT_TRUE(contains_ip(0xC0A801FFU));        // last
    EXPECT_TRUE(contains_ip(0xC0A80100U - 1U));   // outside lower
    EXPECT_TRUE(contains_ip(0xC0A801FFU + 1U));   // outside upper
    EXPECT_TRUE(contains_ip(0xC0A80100U + 128U)); // center
}
