/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

#include <chrono>
#include <limits>
#include <sstream>

#include <gtest/gtest.h>

namespace score
{
namespace td
{

namespace
{

PtpTimeInfo MakePtpTimeInfoWithRateDeviation(const double rate_deviation)
{
    PtpTimeInfo info{};
    info.ptp_assumed_time = std::chrono::nanoseconds{1234};
    info.local_time = PtpTimeInfo::ReferenceClock::time_point{std::chrono::nanoseconds{5678}};
    info.rate_deviation = rate_deviation;
    info.status = {true, false, false, false, true};
    info.sync_fup_data = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    info.pdelay_data = {11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U, 20U, 21U, 22U};
    return info;
}

}  // namespace

TEST(PtpStatusTest, EqualsWhenAllFieldsMatch)
{
    const PtpStatus first{true, false, true, false, true};
    const PtpStatus second{true, false, true, false, true};

    EXPECT_TRUE(first == second);
}

TEST(PtpStatusTest, NotEqualsWhenAnyFieldDiffers)
{
    const PtpStatus first{true, false, true, false, true};
    const PtpStatus second{false, false, true, false, true};

    EXPECT_FALSE(first == second);
}

TEST(SyncFupDataTest, EqualsWhenAllFieldsMatch)
{
    const SyncFupData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    const SyncFupData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};

    EXPECT_TRUE(first == second);
}

TEST(SyncFupDataTest, NotEqualsOperatorReturnsTrueWhenDifferent)
{
    const SyncFupData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    const SyncFupData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 10U};

    EXPECT_TRUE(first != second);
}

TEST(PDelayDataTest, EqualsWhenAllFieldsMatch)
{
    const PDelayData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const PDelayData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};

    EXPECT_TRUE(first == second);
}

TEST(PDelayDataTest, NotEqualsOperatorReturnsTrueWhenDifferent)
{
    const PDelayData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const PDelayData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 13U};

    EXPECT_TRUE(first != second);
}

TEST(PtpTimeInfoTest, EqualsUsesToleranceForRateDeviation)
{
    const PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    const PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0 + (std::numeric_limits<double>::epsilon() / 2.0));

    EXPECT_TRUE(first == second);
}

TEST(PtpTimeInfoTest, NotEqualsWhenRateDeviationOutsideTolerance)
{
    const PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    const PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0 + (std::numeric_limits<double>::epsilon() * 2.0));

    EXPECT_TRUE(first != second);
}

TEST(PtpTimeInfoTest, EqualsWhenAllFieldsMatch)
{
    const PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    const PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0);

    EXPECT_TRUE(first == second);
}

TEST(PtpTimeInfoTest, NotEqualsWhenStatusDiffers)
{
    PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0);
    second.status.is_timeout = !second.status.is_timeout;

    EXPECT_TRUE(first != second);
}

}  // namespace td
}  // namespace score
