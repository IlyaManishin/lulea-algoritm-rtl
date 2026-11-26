#pragma once

#include "gen/gen.hpp"

#include <cstddef>
#include <fstream>
#include <string>

namespace io
{
    struct ConfigEntry
    {
        gen::Mode mode = gen::Mode::Stats;
        std::string input_path;
        std::string dest_dir;
    };

    class ConfigParser
    {
    public:
        explicit ConfigParser(const std::string &filepath);
        ~ConfigParser() = default;

        ConfigParser(const ConfigParser &) = delete;
        ConfigParser &operator=(const ConfigParser &) = delete;

        ConfigParser(ConfigParser &&) = delete;
        ConfigParser &operator=(ConfigParser &&) = delete;

        std::size_t count_remaining();

        // Reads the next record and converts it to a ConfigEntry instance.
        // Returns true if a record was successfully read, false on EOF.
        bool next(ConfigEntry &config_out);

        size_t get_line_number() const { return this->current_line_; }
        const std::string &get_filepath() const { return this->filepath_; }

    private:
        std::string filepath_;
        std::ifstream file_;
        size_t current_line_ = 0;
    };
} // namespace io
