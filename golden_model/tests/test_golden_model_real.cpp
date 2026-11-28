#include <gtest/gtest.h>

#include <cerrno>
#include <filesystem>
#include <memory>
#include <stdlib.h>
#include <string>
#include <system_error>
#include <utility>

#include "gen.hpp"
#include "golden_model_lookup.hpp"
#include <lulea_common/reader.hpp>
#include <lulea_verify/lookup_engine.hpp>
#include <lulea_verify/lookup_verification.hpp>

namespace
{
    class GoldenModelEngineFactory final : public engine::ILookupEngineFactory
    {
    public:
        GoldenModelEngineFactory()
        {
            std::string path = (std::filesystem::temp_directory_path() / "lulea_golden_model_real_tests.XXXXXX").string();
            if (::mkdtemp(path.data()) == nullptr)
            {
                throw std::system_error(errno, std::generic_category(), "Failed to create cache dump directory");
            }
            dump_dir_ = std::move(path);
        }

        ~GoldenModelEngineFactory() override
        {
            std::error_code error;
            std::filesystem::remove_all(dump_dir_, error);
        }

        GoldenModelEngineFactory(const GoldenModelEngineFactory &) = delete;
        GoldenModelEngineFactory &operator=(const GoldenModelEngineFactory &) = delete;
        GoldenModelEngineFactory(GoldenModelEngineFactory &&) = delete;
        GoldenModelEngineFactory &operator=(GoldenModelEngineFactory &&) = delete;

        std::unique_ptr<engine::ILookupEngine> get_lookup_engine(
            const std::filesystem::path &routes_path, const gen::MaskEntries &entries) override
        {
            const std::filesystem::path table_dump = dump_dir_ / routes_path.parent_path().filename();
            gen::build_lulea_cache(entries, gen::Mode::Dump, table_dump);
            return std::make_unique<tests::GoldenModelLookup>(table_dump);
        }

    private:
        std::filesystem::path dump_dir_;
    };
} // namespace

TEST(GoldenModelRealTableTest, VerifyModelWithReference)
{
    GoldenModelEngineFactory factory;
    tests::verify_lookup_engine(factory);
}
