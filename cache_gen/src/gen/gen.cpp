#include "gen.hpp"

#include "lulea_algo/errors.hpp"
#include "lulea_algo/lulea_algo.hpp"
#include "lulea_algo/lulea_tree.hpp"
#include <lulea_common/reader.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace gen
{
    namespace
    {
        void validate_dest_dir(Mode mode, const std::filesystem::path &dest_dir)
        {
            if (mode == Mode::Dump && dest_dir.empty())
            {
                throw FatalGenException("Destination directory is required for Dump mode");
            }
        }
    } // namespace

    CacheStats build_lulea_cache(const MaskEntries &entries,
                                 Mode mode,
                                 const std::filesystem::path &dest_dir)
    {
        validate_dest_dir(mode, dest_dir);

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
            LuleaAlgo algo(std::move(tree));
            if (mode == Mode::Dump)
            {
                return algo.generate_caches(dest_dir);
            }
            return algo.generate_caches();
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

    CacheStats build_lulea_cache(const std::filesystem::path &input_path,
                                 Mode mode,
                                 const std::filesystem::path &dest_dir)
    {
        if (input_path.empty())
        {
            throw FatalGenException("Input path cannot be empty");
        }
        validate_dest_dir(mode, dest_dir);

        MaskEntries entries;
        try
        {
            entries = Reader::read(input_path.string());
        }
        catch (const PortOverflowError &ex)
        {
            throw GenException("Port overflow in route table: " + std::string(ex.what()));
        }
        catch (const std::runtime_error &ex)
        {
            throw GenException("Failed to read route table: " + std::string(ex.what()));
        }

        return build_lulea_cache(entries, mode, dest_dir);
    }

} // namespace gen
