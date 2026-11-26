#include "golden_model_lookup.hpp"

#include "lulea_model.h"
#include <lulea_common/config.hpp>

#include <array>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace tests
{
    GoldenModelLookup::GoldenModelLookup(const std::filesystem::path &cache_dir)
        : model_(nullptr, lulea_model_destroy)
    {
        const std::string path = cache_dir.string();
        std::array<char, 512> error{};
        model_.reset(lulea_model_create(path.c_str(), error.data(), error.size()));
        if (!model_)
        {
            throw std::runtime_error(error.data());
        }
    }

    gen::port_t GoldenModelLookup::lookup(gen::ipv4_t ip) const
    {
        lulea_result_t result = lulea_model_tick(model_.get(), true, ip);
        for (int edge = 1; edge < LULEA_MODEL_STAGES; ++edge)
        {
            if (result.valid)
            {
                throw std::runtime_error("Early golden model response on edge " + std::to_string(edge) +
                                         " for IP " + std::to_string(ip));
            }
            result = lulea_model_tick(model_.get(), false, 0);
        }
        if (!result.valid)
        {
            throw std::runtime_error("No golden model response on edge " + std::to_string(LULEA_MODEL_STAGES) +
                                     " for IP " + std::to_string(ip));
        }
        return static_cast<gen::port_t>(result.port);
    }
} // namespace tests
