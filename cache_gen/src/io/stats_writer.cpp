#include "stats_writer.hpp"

#include "gen/gen.hpp"

#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace io
{

    namespace
    {
        constexpr std::string_view CSV_HEADER =
            "file,l1_pop,l1_chunk,l1_seq,l2_pop,l2_chunk,l2_seq,l3_pop,l3_chunk,l3_seq\n";
    } // namespace

    StatsWriter::StatsWriter(const std::string &csv_filepath)
        : csv_filepath_(csv_filepath)
    {
        if (!this->csv_filepath_.empty())
        {
            this->file_.open(this->csv_filepath_);
            if (!this->file_.is_open())
            {
                throw std::runtime_error("cannot open output CSV: " + this->csv_filepath_);
            }
            this->file_ << CSV_HEADER;
            this->is_csv_ = true;
        }
    }

    StatsWriter::StatsWriter(std::ostream &out, bool is_csv)
        : custom_stream_(&out), is_csv_(is_csv)
    {
        if (this->is_csv_)
        {
            *this->custom_stream_ << CSV_HEADER;
        }
    }

    std::ostream &StatsWriter::get_target_stream() const
    {
        if (this->custom_stream_ != nullptr)
        {
            return *this->custom_stream_;
        }
        if (this->file_.is_open())
        {
            return this->file_;
        }
        return std::cout;
    }

    void StatsWriter::print_stats_stdout(const std::string &input_path, const gen::CacheStats &stats)
    {
        write_stats_stdout(std::cout, input_path, stats);
    }

    void StatsWriter::write_stats_stdout(std::ostream &out, const std::string &input_path, const gen::CacheStats &stats)
    {
        out << input_path << "\t"
            << stats.l1_pop << " " << stats.l1_chunk << " " << stats.l1_seq << " "
            << stats.l2_pop << " " << stats.l2_chunk << " " << stats.l2_seq << " "
            << stats.l3_pop << " " << stats.l3_chunk << " " << stats.l3_seq << "\n";
    }

    void StatsWriter::write_stats_csv(std::ostream &out, const std::string &input_path, const gen::CacheStats &stats)
    {
        out << input_path << ","
            << stats.l1_pop << "," << stats.l1_chunk << "," << stats.l1_seq << ","
            << stats.l2_pop << "," << stats.l2_chunk << "," << stats.l2_seq << ","
            << stats.l3_pop << "," << stats.l3_chunk << "," << stats.l3_seq << "\n";
    }

    void StatsWriter::write(const std::string &input_path, const gen::CacheStats &stats) const
    {
        std::ostream &out = get_target_stream();

        if (this->is_csv_)
        {
            write_stats_csv(out, input_path, stats);
        }
        else
        {
            write_stats_stdout(out, input_path, stats);
        }
    }

    void StatsWriter::write(const gen::Config &config, const gen::CacheStats &stats) const
    {
        this->write(config.input_path, stats);
    }

} // namespace io
