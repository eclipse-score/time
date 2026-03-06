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
#include "score_time/utils/time_utils.hpp"

#include <time.h>

namespace {

using score_time::utils::ClockNs;
using score_time::utils::ClockNsSafe;

// ─── ClockNsSafe ─────────────────────────────────────────────

TEST(ClockNsSafeTest, MonotonicClock_ReturnsValue)
{
    const auto result = ClockNsSafe(CLOCK_MONOTONIC);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(*result, 0);
}

TEST(ClockNsSafeTest, RealtimeClock_ReturnsValue)
{
    const auto result = ClockNsSafe(CLOCK_REALTIME);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(*result, 0);
}

TEST(ClockNsSafeTest, MonotonicClock_NonDecreasing)
{
    const auto t1 = ClockNsSafe(CLOCK_MONOTONIC);
    const auto t2 = ClockNsSafe(CLOCK_MONOTONIC);
    ASSERT_TRUE(t1.has_value());
    ASSERT_TRUE(t2.has_value());
    EXPECT_GE(*t2, *t1);
}

TEST(ClockNsSafeTest, InvalidClockId_ReturnsNullopt)
{
    // clock id 999 is not a valid POSIX clock on Linux
    const auto result = ClockNsSafe(static_cast<clockid_t>(999));
    EXPECT_FALSE(result.has_value());
}

// ─── ClockNs ─────────────────────────────────────────────────

TEST(ClockNsTest, MonotonicClock_ReturnsNonZero)
{
    EXPECT_GT(ClockNs(CLOCK_MONOTONIC), 0);
}

TEST(ClockNsTest, RealtimeClock_ReturnsNonZero)
{
    EXPECT_GT(ClockNs(CLOCK_REALTIME), 0);
}

TEST(ClockNsTest, InvalidClockId_ReturnsZero)
{
    EXPECT_EQ(ClockNs(static_cast<clockid_t>(999)), 0);
}

TEST(ClockNsTest, MonotonicClock_NonDecreasing)
{
    const auto t1 = ClockNs(CLOCK_MONOTONIC);
    const auto t2 = ClockNs(CLOCK_MONOTONIC);
    EXPECT_GE(t2, t1);
}

// ─── Consistency between ClockNsSafe and ClockNs ─────────────

TEST(ClockNsTest, ClockNsSafe_AndClockNs_ReturnSameClock)
{
    // Both should read CLOCK_MONOTONIC close in time
    const auto safe = ClockNsSafe(CLOCK_MONOTONIC);
    const auto plain = ClockNs(CLOCK_MONOTONIC);
    ASSERT_TRUE(safe.has_value());
    // Allow 1ms difference for scheduler jitter
    EXPECT_NEAR(*safe, plain, 1'000'000LL);
}

}  // namespace
