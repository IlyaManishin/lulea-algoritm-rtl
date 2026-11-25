#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>

#include "gen.hpp"
#include "lulea_algo/lulea_algo.hpp"
#include "lulea_algo/lulea_tree.hpp"
#include "lulea_engine/lulea_cache_lookup.hpp"
#include <lulea_common/reader.hpp>

namespace fs = std::filesystem;

namespace
{
    class LuleaCacheLookupTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            test_dir_ = fs::temp_directory_path() / "lulea_cache_lookup_tests";
            fs::create_directories(test_dir_);
        }

        void TearDown() override
        {
            fs::remove_all(test_dir_);
        }

        const fs::path &test_dir() const noexcept
        {
            return test_dir_;
        }

    private:
        fs::path test_dir_;
    };
} // namespace

TEST_F(LuleaCacheLookupTest, WalkAcrossSecondLeafBlock)
{
    const gen::MaskEntries entries{
        {0, std::byte{0}, std::byte{7}},
        {0, std::byte{32}, std::byte{11}},
        {0x00004000, std::byte{24}, std::byte{22}},
        {0x00004040, std::byte{26}, std::byte{255}},
    };
    const gen::Config config{gen::Mode::Dump, "", test_dir().string()};
    gen::LuleaAlgo algo(gen::LuleaTree(entries), config);
    algo.generate_caches(true);
    const engine::LuleaCacheLookup cache(test_dir());
    EXPECT_EQ(cache.lookup(0), std::byte{11});
    EXPECT_EQ(cache.lookup(1), std::byte{7});
    EXPECT_EQ(cache.lookup(0x0000403f), std::byte{22});
    EXPECT_EQ(cache.lookup(0x00004040), std::byte{255});
    EXPECT_EQ(cache.lookup(0x0000407f), std::byte{255});
    EXPECT_EQ(cache.lookup(0x00004080), std::byte{22});
    EXPECT_EQ(cache.lookup(0x00004100), std::byte{7});
    EXPECT_EQ(cache.lookup(0xffffffff), std::byte{7});
}
