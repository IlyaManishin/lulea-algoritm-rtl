#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include "gen/config.hpp"
#include "gen/core/cache.hpp"

TEST(BaseCellTest, DefaultsToZeroAndComputesCapacity)
{
    const gen::BaseCell cell(10);

    EXPECT_EQ(cell.raw(), 0);
    EXPECT_EQ(cell.bit_width(), 10);
    EXPECT_EQ(cell.max_raw(), (size_t{1} << 10) - 1);
}

TEST(BaseCellTest, ValidatesBitWidth)
{
    EXPECT_THROW(gen::BaseCell(0), std::invalid_argument);
    EXPECT_THROW(gen::BaseCell(static_cast<size_t>(std::numeric_limits<size_t>::digits) + 1), std::invalid_argument);

    EXPECT_NO_THROW(gen::BaseCell(1));
    EXPECT_NO_THROW(gen::BaseCell(static_cast<size_t>(std::numeric_limits<size_t>::digits)));
}

TEST(BaseCellTest, EnforcesCapacityOnConstructionAndMutation)
{
    gen::BaseCell one_bit(1);
    EXPECT_EQ(one_bit.max_raw(), 1);
    EXPECT_NO_THROW(one_bit.set_raw(1));
    EXPECT_EQ(one_bit.raw(), 1);
    EXPECT_THROW(one_bit.set_raw(2), gen::LuleaOverflow);

    gen::BaseCell cell(8, 255);
    EXPECT_EQ(cell.raw(), 255);
    EXPECT_THROW(cell.set_raw(256), gen::LuleaOverflow);
    EXPECT_THROW(gen::BaseCell(8, 256), gen::LuleaOverflow);

    const size_t max_bits = static_cast<size_t>(std::numeric_limits<size_t>::digits);
    const size_t max_size_t = std::numeric_limits<size_t>::max();
    const gen::BaseCell full_word(max_bits, max_size_t);
    EXPECT_EQ(full_word.max_raw(), max_size_t);
    EXPECT_EQ(full_word.raw(), max_size_t);
}

TEST(BaseCellTest, SupportsEquality)
{
    const gen::BaseCell a(8, 42);
    const gen::BaseCell b(8, 42);
    const gen::BaseCell c(8, 43);
    const gen::BaseCell d(16, 42);

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
}

TEST(LuleaCellTest, InheritsFromBaseCell)
{
    EXPECT_TRUE((std::is_base_of_v<gen::BaseCell, gen::LuleaCell>));

    const auto cell = gen::LuleaCell::make_ptr(18, 42);
    const gen::BaseCell &base_ref = cell;

    EXPECT_EQ(base_ref.bit_width(), 18);
    EXPECT_EQ(base_ref.raw(), (size_t{1} << 17) | 42);
    EXPECT_EQ(base_ref.max_raw(), (size_t{1} << 18) - 1);
}

TEST(LuleaCellTest, DefaultsToNullPort)
{
    const gen::LuleaCell cell(18);

    EXPECT_TRUE(cell.is_port());
    EXPECT_FALSE(cell.is_ptr());
    EXPECT_TRUE(cell.is_null());
    EXPECT_EQ(cell.port(), gen::NULL_PORT);
    EXPECT_EQ(cell.value(), 0);
    EXPECT_EQ(cell.raw(), 0);
    EXPECT_EQ(cell.bit_width(), 18);
    EXPECT_EQ(cell.max_value(), (size_t{1} << 17) - 1);
}

TEST(LuleaCellTest, ThrowsOnInvalidCellSizeBits)
{
    EXPECT_THROW(gen::LuleaCell(0), std::invalid_argument);
    EXPECT_THROW(gen::LuleaCell(1), std::invalid_argument);
    EXPECT_THROW(gen::LuleaCell(static_cast<size_t>(std::numeric_limits<size_t>::digits) + 1), std::invalid_argument);

    EXPECT_NO_THROW(gen::LuleaCell(2));
    EXPECT_NO_THROW(gen::LuleaCell(static_cast<size_t>(std::numeric_limits<size_t>::digits)));
}

TEST(LuleaCellTest, SetAndGetPort)
{
    gen::LuleaCell cell(18);
    cell.set_port(gen::port_t{255});

    EXPECT_TRUE(cell.is_port());
    EXPECT_FALSE(cell.is_ptr());
    EXPECT_FALSE(cell.is_null());
    EXPECT_EQ(cell.port(), gen::port_t{255});
    EXPECT_EQ(cell.value(), 255);
    EXPECT_EQ(cell.raw(), 255);
    EXPECT_THROW(cell.ptr(), std::logic_error);
}

TEST(LuleaCellTest, SetAndGetPtr)
{
    gen::LuleaCell cell(18);
    cell.set_ptr(12345);

    EXPECT_TRUE(cell.is_ptr());
    EXPECT_FALSE(cell.is_port());
    EXPECT_FALSE(cell.is_null());
    EXPECT_EQ(cell.ptr(), 12345);
    EXPECT_EQ(cell.value(), 12345);
    EXPECT_EQ(cell.raw(), (size_t{1} << 17) | 12345);
    EXPECT_THROW(cell.port(), std::logic_error);
}

TEST(LuleaCellTest, ZeroPtrStillHasPtrFlagSet)
{
    const auto cell = gen::LuleaCell::make_ptr(18, 0);

    EXPECT_TRUE(cell.is_ptr());
    EXPECT_FALSE(cell.is_port());
    EXPECT_FALSE(cell.is_null());
    EXPECT_EQ(cell.ptr(), 0);
    EXPECT_EQ(cell.value(), 0);
    EXPECT_EQ(cell.raw(), size_t{1} << 17);
}

TEST(LuleaCellTest, SwitchingBetweenPortAndPtr)
{
    gen::LuleaCell cell(18);

    cell.set_ptr(500);
    EXPECT_TRUE(cell.is_ptr());
    EXPECT_EQ(cell.ptr(), 500);

    cell.set_port(gen::port_t{7});
    EXPECT_TRUE(cell.is_port());
    EXPECT_EQ(cell.port(), gen::port_t{7});
    EXPECT_EQ(cell.raw(), 7);

    cell.set_port(gen::NULL_PORT);
    EXPECT_TRUE(cell.is_null());
    EXPECT_EQ(cell.raw(), 0);
}

TEST(LuleaCellTest, ThrowsOnPortOverflow)
{
    // 4 total bits = 1 flag bit + 3 payload bits (max = 7)
    gen::LuleaCell cell(4);
    EXPECT_NO_THROW(cell.set_port(gen::port_t{7}));
    EXPECT_EQ(cell.port(), gen::port_t{7});

    EXPECT_THROW(cell.set_port(gen::port_t{8}), gen::LuleaOverflow);
    EXPECT_THROW(gen::LuleaCell(4, gen::port_t{8}), gen::LuleaOverflow);
    EXPECT_THROW(gen::LuleaCell::make_port(4, gen::port_t{255}), gen::LuleaOverflow);

    // Minimal 2-bit cell = 1 flag bit + 1 payload bit (max = 1)
    gen::LuleaCell min_cell(2);
    EXPECT_NO_THROW(min_cell.set_port(gen::port_t{1}));
    EXPECT_THROW(min_cell.set_port(gen::port_t{2}), gen::LuleaOverflow);
}

TEST(LuleaCellTest, ThrowsOnPtrOverflow)
{
    gen::LuleaCell cell(18);
    const size_t max_18 = (size_t{1} << 17) - 1;

    EXPECT_NO_THROW(cell.set_ptr(max_18));
    EXPECT_EQ(cell.ptr(), max_18);
    EXPECT_THROW(cell.set_ptr(max_18 + 1), gen::LuleaOverflow);

    const size_t max_bits = static_cast<size_t>(std::numeric_limits<size_t>::digits);
    gen::LuleaCell wide_cell(max_bits);
    const size_t max_wide = (size_t{1} << (max_bits - 1)) - 1;

    EXPECT_NO_THROW(wide_cell.set_ptr(max_wide));
    EXPECT_EQ(wide_cell.ptr(), max_wide);
    EXPECT_THROW(wide_cell.set_ptr(max_wide + 1), gen::LuleaOverflow);
}

TEST(LuleaRouteTest, InitializesWithDefaultNullPort)
{
    const gen::LuleaRoute route(8, 18);

    EXPECT_EQ(route.size(), 8);
    EXPECT_FALSE(route.empty());
    EXPECT_EQ(route.cell_size_bits(), 18);

    for (size_t i = 0; i < route.size(); ++i)
    {
        EXPECT_TRUE(route[i].is_port());
        EXPECT_TRUE(route[i].is_null());
        EXPECT_EQ(route[i].port(), gen::NULL_PORT);
    }
}

TEST(LuleaRouteTest, InitializesWithCustomDefaultPort)
{
    const gen::LuleaRoute route(4, 18, gen::port_t{42});

    for (const auto &cell : route)
    {
        EXPECT_TRUE(cell.is_port());
        EXPECT_FALSE(cell.is_null());
        EXPECT_EQ(cell.port(), gen::port_t{42});
    }
}

TEST(LuleaRouteTest, ValidatesEvenWhenZeroSize)
{
    EXPECT_THROW(gen::LuleaRoute(0, 1), std::invalid_argument);
    EXPECT_THROW(gen::LuleaRoute(0, 4, gen::port_t{10}), gen::LuleaOverflow);

    const gen::LuleaRoute empty_route(0, 18);
    EXPECT_TRUE(empty_route.empty());
    EXPECT_EQ(empty_route.size(), 0);
}

TEST(LuleaRouteTest, MutatesViaIndexOperator)
{
    gen::LuleaRoute route(3, 18);

    route[0].set_port(gen::port_t{5});
    route[1].set_ptr(1024);

    EXPECT_EQ(route[0].port(), gen::port_t{5});
    EXPECT_TRUE(route[1].is_ptr());
    EXPECT_EQ(route[1].ptr(), 1024);
    EXPECT_TRUE(route[2].is_null());
}

TEST(LuleaRouteTest, IteratorsWorkForReadAndWrite)
{
    gen::LuleaRoute route(4, 18);

    size_t idx = 0;
    for (auto &cell : route)
    {
        cell.set_ptr(idx * 10);
        ++idx;
    }

    idx = 0;
    for (auto it = route.cbegin(); it != route.cend(); ++it)
    {
        EXPECT_TRUE(it->is_ptr());
        EXPECT_EQ(it->ptr(), idx * 10);
        ++idx;
    }
}
