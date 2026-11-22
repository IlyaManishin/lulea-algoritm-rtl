#include <gtest/gtest.h>

#include <cstddef>

#include "io/reader.hpp"
#include "utils/linear_lookup.hpp"
#include "utils/utils.hpp"

TEST(RouteTableReadTest, BinaryAndTextRoutesMatch)
{
    const auto tables = tests::get_test_tables();
    ASSERT_FALSE(tables.empty()) << "No routing tables found";

    for (const auto &table : tables)
    {
        SCOPED_TRACE(table.name);
        const gen::MaskEntries bin_entries = gen::Reader::read(table.bin_path.string());
        const gen::MaskEntries txt_entries = tests::LinearLookup::parse_routes_txt(table.txt_path.string());
        ASSERT_EQ(bin_entries.size(), txt_entries.size());

        // Verify binary and text entries are identical.
        for (size_t idx = 0; idx < txt_entries.size(); ++idx)
        {
            SCOPED_TRACE(idx);
            EXPECT_EQ(bin_entries[idx].ip, txt_entries[idx].ip);
            EXPECT_EQ(bin_entries[idx].mask, txt_entries[idx].mask);
            EXPECT_EQ(bin_entries[idx].port, txt_entries[idx].port);
        }
    }
}
