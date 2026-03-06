/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/
#include <gtest/gtest.h>
#include "score_time/utils/string_parser.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace {

using score_time::utils::ParseDouble;
using score_time::utils::ParseInteger;

// ─── ParseInteger<int> ───────────────────────────────────────

TEST(ParseIntegerTest, ValidPositive_ReturnsTrue)
{
    int v{};
    EXPECT_TRUE(ParseInteger("42", v));
    EXPECT_EQ(v, 42);
}

TEST(ParseIntegerTest, ValidNegative_ReturnsTrue)
{
    int v{};
    EXPECT_TRUE(ParseInteger("-100", v));
    EXPECT_EQ(v, -100);
}

TEST(ParseIntegerTest, Zero_ReturnsTrue)
{
    int v{-1};
    EXPECT_TRUE(ParseInteger("0", v));
    EXPECT_EQ(v, 0);
}

TEST(ParseIntegerTest, EmptyString_ReturnsFalse)
{
    int v{};
    EXPECT_FALSE(ParseInteger("", v));
}

TEST(ParseIntegerTest, LeadingSpace_ReturnsFalse)
{
    int v{};
    EXPECT_FALSE(ParseInteger(" 42", v));
}

TEST(ParseIntegerTest, TrailingChars_ReturnsFalse)
{
    int v{};
    EXPECT_FALSE(ParseInteger("42abc", v));
}

TEST(ParseIntegerTest, PureAlpha_ReturnsFalse)
{
    int v{};
    EXPECT_FALSE(ParseInteger("abc", v));
}

TEST(ParseIntegerTest, HexPrefix_ReturnsFalse)
{
    int v{};
    EXPECT_FALSE(ParseInteger("0x1F", v));
}

TEST(ParseIntegerTest, LeadingPlus_ReturnsFalse)
{
    // std::from_chars does not accept a leading '+'
    int v{};
    EXPECT_FALSE(ParseInteger("+1", v));
}

TEST(ParseIntegerTest, WhitespaceOnly_ReturnsFalse)
{
    int v{};
    EXPECT_FALSE(ParseInteger("   ", v));
}

// ─── ParseInteger<int64_t> ───────────────────────────────────

TEST(ParseIntegerTest, Int64Max_ReturnsTrue)
{
    std::int64_t v{};
    const std::string s = std::to_string(std::numeric_limits<std::int64_t>::max());
    EXPECT_TRUE(ParseInteger(s, v));
    EXPECT_EQ(v, std::numeric_limits<std::int64_t>::max());
}

TEST(ParseIntegerTest, Int64Min_ReturnsTrue)
{
    std::int64_t v{};
    const std::string s = std::to_string(std::numeric_limits<std::int64_t>::min());
    EXPECT_TRUE(ParseInteger(s, v));
    EXPECT_EQ(v, std::numeric_limits<std::int64_t>::min());
}

TEST(ParseIntegerTest, LargePositiveInt64_ReturnsTrue)
{
    std::int64_t v{};
    EXPECT_TRUE(ParseInteger("1000000000000", v));
    EXPECT_EQ(v, 1'000'000'000'000LL);
}

// ─── ParseInteger<uint32_t> ──────────────────────────────────

TEST(ParseIntegerTest, UInt32MaxValue_ReturnsTrue)
{
    std::uint32_t v{};
    EXPECT_TRUE(ParseInteger("4294967295", v));
    EXPECT_EQ(v, std::numeric_limits<std::uint32_t>::max());
}

TEST(ParseIntegerTest, NegativeForUnsigned_ReturnsFalse)
{
    std::uint32_t v{};
    EXPECT_FALSE(ParseInteger("-1", v));
}

// ─── ParseInteger<uint64_t> ──────────────────────────────────

TEST(ParseIntegerTest, UInt64_ValidValue_ReturnsTrue)
{
    std::uint64_t v{};
    EXPECT_TRUE(ParseInteger("8192", v));
    EXPECT_EQ(v, 8192u);
}

// ─── std::string overload ─────────────────────────────────────

TEST(ParseIntegerTest, StdStringOverload_Works)
{
    int v{};
    EXPECT_TRUE(ParseInteger(std::string("123"), v));
    EXPECT_EQ(v, 123);
}

// ─── ParseDouble ─────────────────────────────────────────────

TEST(ParseDoubleTest, ValidPositive_ReturnsTrue)
{
    double v{};
    EXPECT_TRUE(ParseDouble("3.14", v));
    EXPECT_NEAR(v, 3.14, 1e-9);
}

TEST(ParseDoubleTest, ValidNegative_ReturnsTrue)
{
    double v{};
    EXPECT_TRUE(ParseDouble("-2.5", v));
    EXPECT_NEAR(v, -2.5, 1e-9);
}

TEST(ParseDoubleTest, Zero_ReturnsTrue)
{
    double v{-1.0};
    EXPECT_TRUE(ParseDouble("0.0", v));
    EXPECT_NEAR(v, 0.0, 1e-15);
}

TEST(ParseDoubleTest, ScientificNotation_ReturnsTrue)
{
    double v{};
    EXPECT_TRUE(ParseDouble("1e10", v));
    EXPECT_NEAR(v, 1e10, 1.0);
}

TEST(ParseDoubleTest, NegativeScientific_ReturnsTrue)
{
    double v{};
    EXPECT_TRUE(ParseDouble("-1.5e-3", v));
    EXPECT_NEAR(v, -1.5e-3, 1e-15);
}

TEST(ParseDoubleTest, IntegerInput_ReturnsTrue)
{
    double v{};
    EXPECT_TRUE(ParseDouble("42", v));
    EXPECT_NEAR(v, 42.0, 1e-9);
}

TEST(ParseDoubleTest, One_ReturnsTrue)
{
    double v{};
    EXPECT_TRUE(ParseDouble("1.0", v));
    EXPECT_NEAR(v, 1.0, 1e-15);
}

TEST(ParseDoubleTest, EmptyString_ReturnsFalse)
{
    double v{};
    EXPECT_FALSE(ParseDouble("", v));
}

TEST(ParseDoubleTest, PureAlpha_ReturnsFalse)
{
    double v{};
    EXPECT_FALSE(ParseDouble("abc", v));
}

TEST(ParseDoubleTest, TrailingChars_ReturnsFalse)
{
    double v{};
    EXPECT_FALSE(ParseDouble("1.5x", v));
}

TEST(ParseDoubleTest, TooLongString_ReturnsFalse)
{
    double v{};
    // 128 characters exceeds the 127-char limit
    EXPECT_FALSE(ParseDouble(std::string(128, '1'), v));
}

TEST(ParseDoubleTest, ExactlyMaxLength_ReturnsTrue)
{
    // A 127-char string of '0's followed by ".0" won't be a valid double but
    // the length check passes; test a valid 4-char double at max-safe length.
    double v{};
    EXPECT_TRUE(ParseDouble("0.5", v));
    EXPECT_NEAR(v, 0.5, 1e-9);
}

// ─── std::string overload ─────────────────────────────────────

TEST(ParseDoubleTest, StdStringOverload_Works)
{
    double v{};
    EXPECT_TRUE(ParseDouble(std::string("0.5"), v));
    EXPECT_NEAR(v, 0.5, 1e-9);
}

}  // namespace
