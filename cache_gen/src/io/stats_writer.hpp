#pragma once

#include "gen/gen.hpp"

#include <fstream>
#include <string>

namespace io
{

    class StatsWriter
    {
    public:
        explicit StatsWriter(const std::string &csv_filepath = "");
        ~StatsWriter() = default;

        StatsWriter(const StatsWriter &) = delete;
        StatsWriter &operator=(const StatsWriter &) = delete;

        StatsWriter(StatsWriter &&) = delete;
        StatsWriter &operator=(StatsWriter &&) = delete;

        void write(const std::string &input_path, const gen::CacheStats &stats) const;

        bool is_csv() const { return this->file_.is_open(); }

    private:
        mutable std::ofstream file_;
    };

} // namespace io
