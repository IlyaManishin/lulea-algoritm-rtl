#include "stats_writer.hpp"

#include "gen/gen.hpp"

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

        void write_stats_csv(std::ostream &out, const std::string &input_path, const gen::CacheByteSizes &sizes)
        {
            out << input_path << ","
                << sizes.l1_pop << "," << sizes.l1_chunk << "," << sizes.l1_seq << ","
                << sizes.l2_pop << "," << sizes.l2_chunk << "," << sizes.l2_seq << ","
                << sizes.l3_pop << "," << sizes.l3_chunk << "," << sizes.l3_seq << "\n";
        }

        void write_stats_stdout(const std::string &input_path, const gen::CacheByteSizes &sizes)
        {
            std::cout << input_path << "\t"
                      << sizes.l1_pop << " " << sizes.l1_chunk << " " << sizes.l1_seq << " "
                      << sizes.l2_pop << " " << sizes.l2_chunk << " " << sizes.l2_seq << " "
                      << sizes.l3_pop << " " << sizes.l3_chunk << " " << sizes.l3_seq << "\n";
        }
    } // namespace

    StatsWriter::StatsWriter(const std::string &csv_filepath)
    {
        if (!csv_filepath.empty())
        {
            this->file_.open(csv_filepath);
            if (!this->file_.is_open())
            {
                throw std::runtime_error("cannot open output CSV: " + csv_filepath);
            }
            this->file_ << CSV_HEADER;
        }
    }

    void StatsWriter::write(const std::string &input_path, const gen::CacheStats &stats) const
    {
        if (this->file_.is_open())
        {
            write_stats_csv(this->file_, input_path, stats.cache_sizes);
        }
        else
        {
            write_stats_stdout(input_path, stats.cache_sizes);
        }
    }

} // namespace io
