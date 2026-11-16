#include "gen.hpp"

#include "core/errors.hpp"
#include "core/lulea_algo.hpp"
#include "core/lulea_tree.hpp"
#include "io/reader.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace gen
{
    namespace
    {
        bool validate_config(const Config &config, std::string &error_msg)
        {
            if (config.input_path.empty())
            {
                error_msg = "Input path cannot be empty";
                return false;
            }

            if (config.mode == Mode::Dump && config.dest_dir.empty())
            {
                error_msg = "Destination directory is required for Dump mode";
                return false;
            }

            return true;
        }
    } // namespace

    BuildResult build_lulea_cache(const Config &config)
    {
        std::string config_error;
        if (!validate_config(config, config_error))
        {
            return BuildResult{
                .success = false,
                .is_fatal = true,
                .error_message = config_error,
                .stats = {}};
        }

        MaskEntries entries;
        try
        {
            Reader reader(config.input_path);
            entries = reader.read();
        }
        catch (const PortOverflowError &ex)
        {
            return BuildResult{
                .success = false,
                .is_fatal = false,
                .error_message = "Port overflow in route table: " + std::string(ex.what()),
                .stats = {}};
        }
        catch (const std::exception &ex)
        {
            return BuildResult{
                .success = false,
                .is_fatal = false,
                .error_message = "Failed to read route table: " + std::string(ex.what()),
                .stats = {}};
        }

        LuleaTree tree;
        try
        {
            tree.build(entries);
        }
        catch (const LuleaOverflow &ex)
        {
            return BuildResult{
                .success = false,
                .is_fatal = false,
                .error_message = "Lulea tree overflow: " + std::string(ex.what()),
                .stats = {}};
        }
        catch (const std::invalid_argument &ex)
        {
            return BuildResult{
                .success = false,
                .is_fatal = false,
                .error_message = "Invalid route prefix during tree construction: " + std::string(ex.what()),
                .stats = {}};
        }
        catch (const std::exception &ex)
        {
            return BuildResult{
                .success = false,
                .is_fatal = false,
                .error_message = "Failed to build Lulea tree: " + std::string(ex.what()),
                .stats = {}};
        }

        try
        {
            LuleaAlgo algo(std::move(tree), config);
            const CacheStats stats = algo.generate_caches();

            return BuildResult{
                .success = true,
                .is_fatal = false,
                .error_message = "",
                .stats = stats};
        }
        catch (const LuleaOverflow &ex)
        {
            return BuildResult{
                .success = false,
                .is_fatal = false,
                .error_message = "Lulea cache overflow: " + std::string(ex.what()),
                .stats = {}};
        }
        catch (const std::exception &ex)
        {
            return BuildResult{
                .success = false,
                .is_fatal = false,
                .error_message = "Failed to generate caches: " + std::string(ex.what()),
                .stats = {}};
        }
    }

} // namespace gen
