#pragma once

#include <cstddef>

namespace io
{
    class ProgressBar
    {
    public:
        ProgressBar(std::size_t total, bool redraw);
        ~ProgressBar();

        ProgressBar(const ProgressBar &) = delete;
        ProgressBar &operator=(const ProgressBar &) = delete;
        ProgressBar(ProgressBar &&) = delete;
        ProgressBar &operator=(ProgressBar &&) = delete;

        void advance();
        void end_line();

    private:
        static constexpr std::size_t width = 25;

        void render();

        std::size_t total_;
        bool redraw_;
        std::size_t completed_ = 0;
        std::size_t last_filled_ = width + 1;
        bool line_open_ = false;
    };
} // namespace io
