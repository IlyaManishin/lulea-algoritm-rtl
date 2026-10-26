#pragma once

#include <string>

namespace gen {

enum class Mode {
    Stats,
    Dump
};

class Config {
public:
    Config() = default;
    Config(Mode mode, std::string input_path, std::string dest_dir = "");

    Mode get_mode() const { return this->mode; }
    void set_mode(Mode mode) { this->mode = mode; }

    const std::string& get_input_path() const { return this->input_path; }
    void set_input_path(std::string input_path) { this->input_path = std::move(input_path); }

    const std::string& get_dest_dir() const { return this->dest_dir; }
    void set_dest_dir(std::string dest_dir) { this->dest_dir = std::move(dest_dir); }

private:
    Mode mode = Mode::Stats;
    std::string input_path;
    std::string dest_dir;
};

} // namespace gen
