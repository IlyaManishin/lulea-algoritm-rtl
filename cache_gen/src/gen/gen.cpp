#include "gen.hpp"

#include "io/reader.hpp"
#include "lulea_algo/errors.hpp"
#include "lulea_algo/lulea_algo.hpp"
#include "lulea_algo/lulea_tree.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace gen
{
    namespace
    {
        void validate_config(const Config &config)
        {
            if (config.input_path.empty())
            {
                throw FatalGenException("Input path cannot be empty");
            }

            if (config.mode == Mode::Dump && config.dest_dir.empty())
            {
                throw FatalGenException("Destination directory is required for Dump mode");
            }
        }
    } // namespace

    BuildResult build_lulea_cache(const Config &config)
    {
        validate_config(config);

        MaskEntries entries;
        try
        {
            Reader reader(config.input_path);
            entries = reader.read();
        }
        catch (const PortOverflowError &ex)
        {
            throw GenException("Port overflow in route table: " + std::string(ex.what()));
        }
        catch (const std::runtime_error &ex)
        {
            throw GenException("Failed to read route table: " + std::string(ex.what()));
        }

        LuleaTree tree;
        try
        {
            tree.build(entries);
        }
        catch (const LuleaOverflow &ex)
        {
            throw GenException("Lulea tree overflow: " + std::string(ex.what()));
        }
        catch (const std::invalid_argument &ex)
        {
            throw GenException("Invalid route prefix during tree construction: " + std::string(ex.what()));
        }
        catch (const std::runtime_error &ex)
        {
            throw GenException("Failed to build Lulea tree: " + std::string(ex.what()));
        }

        try
        {
            LuleaAlgo algo(std::move(tree), config);
            const CacheStats stats = algo.generate_caches();

            return BuildResult{.stats = stats};
        }
        catch (const LuleaOverflow &ex)
        {
            throw GenException("Lulea cache overflow: " + std::string(ex.what()));
        }
        catch (const std::runtime_error &ex)
        {
            throw GenException("Failed to generate caches: " + std::string(ex.what()));
        }
    }

} // namespace gen
