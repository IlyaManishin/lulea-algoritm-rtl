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
            "file,l1_pop,l1_chunk,l1_seq,l2_pop,l2_chunk,l2_seq,l3_pop,l3_chunk,l3_seq,"
            "l1_pop_aligned,l1_chunk_aligned,l1_seq_aligned,l2_pop_aligned,l2_chunk_aligned,l2_seq_aligned,l3_pop_aligned,l3_chunk_aligned,l3_seq_aligned\n";

        void write_sizes(std::ostream &out, const gen::CacheByteSizes &sizes, char sep)
        {
            out << sizes.l1_pop << sep << sizes.l1_chunk << sep << sizes.l1_seq << sep
                << sizes.l2_pop << sep << sizes.l2_chunk << sep << sizes.l2_seq << sep
                << sizes.l3_pop << sep << sizes.l3_chunk << sep << sizes.l3_seq;
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
            this->file_.exceptions(std::ios::badbit | std::ios::failbit);
            this->file_ << CSV_HEADER;
        }
    }

    void StatsWriter::write(const std::string &input_path, const gen::CacheStats &stats) const
    {
        const bool csv = this->file_.is_open();
        std::ostream &out = csv ? static_cast<std::ostream &>(this->file_) : std::cout;
        const char sep = csv ? ',' : ' ';
        if (csv && input_path.find_first_of(",\"\r\n") != std::string::npos)
        {
            out << '"';
            for (const char ch : input_path)
            {
                if (ch == '"')
                {
                    out << '"';
                }
                out << ch;
            }
            out << '"';
        }
        else
        {
            out << input_path;
        }
        out << (csv ? ',' : '\t');
        write_sizes(out, stats.cache_sizes, sep);
        out << (csv ? ',' : '\t');
        write_sizes(out, stats.cache_sizes_aligned, sep);
        if (!(out << '\n'))
        {
            throw std::runtime_error("Failed to write cache statistics");
        }
    }
} // namespace io
