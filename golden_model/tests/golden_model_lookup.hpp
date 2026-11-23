#pragma once

#include "lulea_model.h"
#include <lulea_common/config.hpp>
#include <lulea_verify/lookup_engine.hpp>

#include <filesystem>
#include <memory>

namespace tests
{
    class GoldenModelLookup final : public engine::ILookupEngine
    {
    public:
        explicit GoldenModelLookup(const std::filesystem::path &cache_dir);
        gen::port_t lookup(gen::ipv4_t ip) const override;

    private:
        std::unique_ptr<lulea_model_t, decltype(&lulea_model_destroy)> model_;
    };
} // namespace tests
