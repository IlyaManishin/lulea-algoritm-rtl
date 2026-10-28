#include "parser.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

ConfigParser::ConfigParser(const std::string& filepath)
    : filepath_(filepath), file_(filepath) {
    if (!this->file_.is_open()) {
        throw std::runtime_error("Cannot open config file: " + filepath);
    }
}

bool ConfigParser::next(gen::Config& config_out) {
    std::string line;

    while (std::getline(this->file_, line)) {
        this->current_line_++;

        // Ignore empty lines and comment lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string mode_str, in_path, dest_dir;

        if (!std::getline(ss, mode_str, '\t') || !std::getline(ss, in_path, '\t')) {
            throw std::runtime_error(this->filepath_ + ":" + std::to_string(this->current_line_) + 
                                     ": malformed config entry (expected tab-separated fields)");
        }

        // Optional 3rd field: dest_dir
        std::getline(ss, dest_dir, '\t');

        gen::Mode mode;
        if (mode_str == "stats") {
            mode = gen::Mode::Stats;
        } else if (mode_str == "dump") {
            mode = gen::Mode::Dump;
        } else {
            throw std::runtime_error(this->filepath_ + ":" + std::to_string(this->current_line_) + 
                                     ": unknown mode '" + mode_str + "' (expected 'stats' or 'dump')");
        }

        // '-' means no destination directory
        if (dest_dir == "-") {
            dest_dir.clear();
        }

        config_out = gen::Config{mode, std::move(in_path), std::move(dest_dir)};
        return true;
    }

    return false;
}
