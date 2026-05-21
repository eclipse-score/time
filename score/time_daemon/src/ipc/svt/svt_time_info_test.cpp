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
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

#include <chrono>
#include <limits>

#include <gtest/gtest.h>

namespace score
{
namespace td
{

TEST(TimeBaseStatusTest, EqualsWhenAllFieldsMatch)
{
    const svt::TimeBaseStatus first{true, false, true, false, true};
    const svt::TimeBaseStatus second{true, false, true, false, true};

    EXPECT_TRUE(first == second);
}

TEST(TimeBaseStatusTest, NotEqualsWhenAnyFieldDiffers)
{
    const svt::TimeBaseStatus first{true, false, true, false, true};
    const svt::TimeBaseStatus second{false, false, true, false, true};

    EXPECT_FALSE(first == second);
}

TEST(SyncFupSnapshotTest, EqualsWhenAllFieldsMatch)
{
    const svt::SyncFupSnapshot first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    const svt::SyncFupSnapshot second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};

    EXPECT_TRUE(first == second);
}

TEST(SyncFupSnapshotTest, NotEqualsOperatorReturnsTrueWhenDifferent)
{
    const svt::SyncFupSnapshot first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    const svt::SyncFupSnapshot second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 10U};

    EXPECT_TRUE(first != second);
}

TEST(PDelayDataSnapshotTest, EqualsWhenAllFieldsMatch)
{
    const svt::PDelayDataSnapshot first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const svt::PDelayDataSnapshot second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};

    EXPECT_TRUE(first == second);
}

TEST(PDelayDataSnapshotTest, NotEqualsOperatorReturnsTrueWhenDifferent)
{
    const svt::PDelayDataSnapshot first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const svt::PDelayDataSnapshot second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 13U};

    EXPECT_TRUE(first != second);
}

TEST(TimeBaseSnapshotTest, CreateFromCopiesAndConvertsFields)
{
    PtpTimeInfo info{};
    info.ptp_assumed_time = std::chrono::nanoseconds{1234};
    info.local_time = PtpTimeInfo::ReferenceClock::time_point{std::chrono::nanoseconds{5678}};
    info.rate_deviation = 1.0;
    info.status = {true, false, true, false, true};
    info.sync_fup_data = {11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U};
    info.pdelay_data = {21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U};

    svt::TimeBaseSnapshot snapshot{};
    snapshot.CreateFrom(info);

    EXPECT_EQ(snapshot.ptp_assumed_time, 1234U);
    EXPECT_EQ(snapshot.local_time, 5678U);
    EXPECT_DOUBLE_EQ(snapshot.rate_deviation, 1.0);
    EXPECT_TRUE((snapshot.status == svt::TimeBaseStatus{true, false, true, false, true}));
    EXPECT_TRUE((snapshot.sync_fup_data == svt::SyncFupSnapshot{11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U}));
    EXPECT_TRUE(
        (snapshot.pdelay_data == svt::PDelayDataSnapshot{21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U}));
}

TEST(TimeBaseSnapshotTest, EqualsUsesToleranceForRateDeviation)
{
    svt::TimeBaseSnapshot first{};
    first.ptp_assumed_time = 1U;
    first.local_time = 2U;
    first.rate_deviation = 1.0;
    first.status = {true, false, false, false, true};
    first.sync_fup_data = {1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U};
    first.pdelay_data = {1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U};

    svt::TimeBaseSnapshot second = first;
    second.rate_deviation = 1.0 + (std::numeric_limits<double>::epsilon() / 2.0);

    EXPECT_TRUE(first == second);
}

TEST(TimeBaseSnapshotTest, NotEqualsWhenRateDeviationOutsideTolerance)
{
    svt::TimeBaseSnapshot first{};
    first.ptp_assumed_time = 1U;
    first.local_time = 2U;
    first.rate_deviation = 1.0;
    first.status = {true, false, false, false, true};
    first.sync_fup_data = {1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U};
    first.pdelay_data = {1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U};

    svt::TimeBaseSnapshot second = first;
    second.rate_deviation = 1.0 + (std::numeric_limits<double>::epsilon() * 2.0);

    EXPECT_TRUE(first != second);
}

TEST(TimeBaseSnapshotTest, EqualsPtpTimeInfoWhenAllFieldsMatch)
{
    PtpTimeInfo info{};
    info.ptp_assumed_time = std::chrono::nanoseconds{1234};
    info.local_time = PtpTimeInfo::ReferenceClock::time_point{std::chrono::nanoseconds{5678}};
    info.rate_deviation = 1.0;
    info.status = {true, false, true, false, true};
    info.sync_fup_data = {11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U};
    info.pdelay_data = {21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U};

    svt::TimeBaseSnapshot snapshot{};
    snapshot.ptp_assumed_time = 1234U;
    snapshot.local_time = 5678U;
    snapshot.rate_deviation = 1.0;
    snapshot.status = {true, false, true, false, true};
    snapshot.sync_fup_data = {11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U};
    snapshot.pdelay_data = {21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U};

    EXPECT_TRUE(snapshot == info);
}

TEST(TimeBaseSnapshotTest, NotEqualsPtpTimeInfoWhenRateDeviationOutsideTolerance)
{
    PtpTimeInfo info{};
    info.ptp_assumed_time = std::chrono::nanoseconds{1234};
    info.local_time = PtpTimeInfo::ReferenceClock::time_point{std::chrono::nanoseconds{5678}};
    info.rate_deviation = 1.0;
    info.status = {true, false, true, false, true};
    info.sync_fup_data = {11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U};
    info.pdelay_data = {21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U};

    svt::TimeBaseSnapshot snapshot{};
    snapshot.ptp_assumed_time = 1234U;
    snapshot.local_time = 5678U;
    snapshot.rate_deviation = 1.0 + (std::numeric_limits<double>::epsilon() * 2.0);
    snapshot.status = {true, false, true, false, true};
    snapshot.sync_fup_data = {11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U};
    snapshot.pdelay_data = {21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U};

    EXPECT_TRUE(snapshot != info);
}

}  // namespace td
}  // namespace score
