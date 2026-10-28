#pragma once

#include <fstream>
#include <string>

#include "gen/gen.hpp"

class ConfigParser {
public:
    explicit ConfigParser(const std::string& filepath);
    ~ConfigParser() = default;

    ConfigParser(const ConfigParser&) = delete;
    ConfigParser& operator=(const ConfigParser&) = delete;

    // Reads the next record and converts it to a gen::Config instance.
    // Returns true if a record was successfully read, false on EOF.
    bool next(gen::Config& config_out);

    size_t get_line_number() const { return this->current_line_; }
    const std::string& get_filepath() const { return this->filepath_; }

private:
    std::string filepath_;
    std::ifstream file_;
    size_t current_line_ = 0;
};
