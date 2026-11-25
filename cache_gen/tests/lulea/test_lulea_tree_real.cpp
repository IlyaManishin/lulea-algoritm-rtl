#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include "lulea_algo/lulea_tree.hpp"
#include "lulea_engine/lulea_tree_lookup.hpp"
#include <lulea_common/config.hpp>
#include <lulea_common/reader.hpp>
#include <lulea_verify/lookup_engine.hpp>
#include <lulea_verify/lookup_verification.hpp>

namespace
{
    class TreeEngine final : public engine::ILookupEngine
    {
    public:
        explicit TreeEngine(const gen::MaskEntries &entries)
            : tree_(std::make_unique<const gen::LuleaTree>(entries)), lookup_(*tree_) {}

        gen::port_t lookup(gen::ipv4_t ip) const override
        {
            return lookup_.lookup(ip);
        }

    private:
        std::unique_ptr<const gen::LuleaTree> tree_;
        engine::LuleaTreeLookup lookup_;
    };

    class TreeEngineFactory final : public engine::ILookupEngineFactory
    {
    public:
        std::unique_ptr<engine::ILookupEngine> get_lookup_engine(
            [[maybe_unused]] const std::filesystem::path &routes_path, const gen::MaskEntries &entries) override
        {
            return std::make_unique<TreeEngine>(entries);
        }
    };
} // namespace

TEST(LuleaTreeRealTableTest, VerifyTreeWithReference)
{
    TreeEngineFactory factory;
    tests::verify_lookup_engine(factory);
}
