#include "progress_bar.hpp"

#include <cstddef>
#include <iostream>
#include <string>

namespace io
{
    ProgressBar::ProgressBar(std::size_t total, bool redraw) : total_(total), redraw_(redraw)
    {
        render();
    }

    ProgressBar::~ProgressBar()
    {
        end_line();
    }

    void ProgressBar::advance()
    {
        if (completed_ < total_)
        {
            ++completed_;
            render();
        }
    }

    void ProgressBar::end_line()
    {
        if (line_open_)
        {
            std::cerr << '\n';
            line_open_ = false;
        }
    }

    void ProgressBar::render()
    {
        if (total_ == 0)
        {
            return;
        }

        const std::size_t filled = completed_ * width / total_;
        if (filled == last_filled_)
        {
            return;
        }

        if (redraw_)
        {
            std::cerr << '\r';
        }
        std::cerr << '[' << std::string(filled, '=') << std::string(width - filled, ' ')
                  << "] " << completed_ << '/' << total_;
        last_filled_ = filled;
        line_open_ = true;
        if (!redraw_)
        {
            end_line();
        }
    }
} // namespace io
