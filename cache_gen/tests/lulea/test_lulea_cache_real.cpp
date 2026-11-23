#include <gtest/gtest.h>

#include <cerrno>
#include <filesystem>
#include <memory>
#include <stdlib.h>
#include <string>
#include <system_error>

#include "engine/lookup_engine.hpp"
#include "engine/lulea_cache_lookup.hpp"
#include "gen.hpp"
#include "io/reader.hpp"
#include "utils/lookup_verification.hpp"

namespace fs = std::filesystem;

namespace
{
    fs::path create_dump_dir()
    {
        std::string path = (fs::temp_directory_path() / "lulea_cache_real_tests.XXXXXX").string();
        if (::mkdtemp(path.data()) == nullptr)
        {
            throw std::system_error(errno, std::generic_category(), "Failed to create cache dump directory");
        }
        return fs::path(path);
    }

    class CacheEngineFactory final : public engine::ILookupEngineFactory
    {
    public:
        CacheEngineFactory() : dump_dir_(create_dump_dir()) {}

        ~CacheEngineFactory() override
        {
            std::error_code error;
            fs::remove_all(dump_dir_, error);
        }

        CacheEngineFactory(const CacheEngineFactory &) = delete;
        CacheEngineFactory &operator=(const CacheEngineFactory &) = delete;
        CacheEngineFactory(CacheEngineFactory &&) = delete;
        CacheEngineFactory &operator=(CacheEngineFactory &&) = delete;

        std::unique_ptr<engine::ILookupEngine> get_lookup_engine(
            const fs::path &routes_path, [[maybe_unused]] const gen::MaskEntries &entries) override
        {
            const fs::path table_dump = dump_dir_ / routes_path.parent_path().filename();
            const gen::Config config{
                .mode = gen::Mode::Dump,
                .input_path = routes_path.string(),
                .dest_dir = table_dump.string(),
            };

            gen::build_lulea_cache(config);
            return std::make_unique<engine::LuleaCacheLookup>(table_dump);
        }

    private:
        fs::path dump_dir_;
    };
} // namespace

TEST(LuleaCacheRealTableTest, VerifyCachesWithReference)
{
    CacheEngineFactory factory;
    tests::verify_lookup_engine(factory);
}
