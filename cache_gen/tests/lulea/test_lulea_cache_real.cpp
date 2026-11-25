#include <gtest/gtest.h>

#include <cerrno>
#include <filesystem>
#include <memory>
#include <stdlib.h>
#include <string>
#include <system_error>
#include <utility>

#include "gen.hpp"
#include "lulea_engine/lulea_cache_lookup.hpp"
#include <lulea_common/reader.hpp>
#include <lulea_verify/lookup_engine.hpp>
#include <lulea_verify/lookup_verification.hpp>

namespace
{
    class CacheEngineFactory final : public engine::ILookupEngineFactory
    {
    public:
        CacheEngineFactory()
        {
            std::string path = (std::filesystem::temp_directory_path() / "lulea_cache_real_tests.XXXXXX").string();
            if (::mkdtemp(path.data()) == nullptr)
            {
                throw std::system_error(errno, std::generic_category(), "Failed to create cache dump directory");
            }
            dump_dir_ = std::move(path);
        }

        ~CacheEngineFactory() override
        {
            std::error_code error;
            std::filesystem::remove_all(dump_dir_, error);
        }

        CacheEngineFactory(const CacheEngineFactory &) = delete;
        CacheEngineFactory &operator=(const CacheEngineFactory &) = delete;
        CacheEngineFactory(CacheEngineFactory &&) = delete;
        CacheEngineFactory &operator=(CacheEngineFactory &&) = delete;

        std::unique_ptr<engine::ILookupEngine> get_lookup_engine(
            const std::filesystem::path &routes_path, [[maybe_unused]] const gen::MaskEntries &entries) override
        {
            const std::filesystem::path table_dump = dump_dir_ / routes_path.parent_path().filename();
            const gen::Config config{
                .mode = gen::Mode::Dump,
                .input_path = routes_path.string(),
                .dest_dir = table_dump.string(),
            };

            gen::build_lulea_cache(config);
            return std::make_unique<engine::LuleaCacheLookup>(table_dump);
        }

    private:
        std::filesystem::path dump_dir_;
    };
} // namespace

TEST(LuleaCacheRealTableTest, VerifyCachesWithReference)
{
    CacheEngineFactory factory;
    tests::verify_lookup_engine(factory);
}
