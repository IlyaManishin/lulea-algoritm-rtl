#pragma once

#include "gen/gen.hpp"

#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

namespace io
{

    class StatsWriter
    {
    public:
        StatsWriter() = default;
        explicit StatsWriter(const std::string &csv_filepath);
        explicit StatsWriter(std::ostream &out, bool is_csv = true);
        ~StatsWriter() = default;

        StatsWriter(const StatsWriter &) = delete;
        StatsWriter &operator=(const StatsWriter &) = delete;

        StatsWriter(StatsWriter &&) = delete;
        StatsWriter &operator=(StatsWriter &&) = delete;

        void write(const std::string &input_path, const gen::CacheStats &stats) const;
        void write(const gen::Config &config, const gen::CacheStats &stats) const;

        static void print_stats_stdout(const std::string &input_path, const gen::CacheStats &stats);
        static void write_stats_stdout(std::ostream &out, const std::string &input_path, const gen::CacheStats &stats);
        static void write_stats_csv(std::ostream &out, const std::string &input_path, const gen::CacheStats &stats);

        bool is_csv() const { return this->is_csv_; }
        const std::string &get_csv_filepath() const { return this->csv_filepath_; }

    private:
        std::ostream &get_target_stream() const;

        std::string csv_filepath_;
        mutable std::ofstream file_;
        std::ostream *custom_stream_ = nullptr;
        bool is_csv_ = false;
    };

} // namespace io
