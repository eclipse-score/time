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
#include "score/time/hirs_time/src/hirs_clock_backend_mock.h"
#include "score/time/clock/src/scoped_clock_override.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

using ::testing::Return;

namespace score
{
namespace time
{

// ── UC5: HirsClock::Now().time_point + duration arithmetic ───────────────────

TEST(HirsClockTest, NowReturnsTimepointSuitableForDurationArithmetic)
{
    auto mock = std::make_shared<HirsClockBackendMock>();
    test_utils::ScopedClockOverride<HirsTime> guard{mock};

    const HirsTime::Timepoint tp{std::chrono::nanoseconds{1'000'000LL}};
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<HirsTime::Timepoint, NoStatus>{tp, NoStatus{}}));

    const auto result = HirsClock::GetInstance().Now();
    const auto deadline = result.TimePoint() + std::chrono::seconds{3};

    EXPECT_EQ(deadline.time_since_epoch(),
              std::chrono::nanoseconds{1'000'000LL} + std::chrono::seconds{3});
}

TEST(HirsClockTest, NowReturnsZeroTimepointByDefault)
{
    auto mock = std::make_shared<HirsClockBackendMock>();
    test_utils::ScopedClockOverride<HirsTime> guard{mock};

    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{}, NoStatus{}}));

    const auto result = HirsClock::GetInstance().Now();

    EXPECT_EQ(result.TimePoint().time_since_epoch(), std::chrono::nanoseconds{0});
}

TEST(HirsClockTest, NowSnapshotCarriesNoStatus)
{
    auto mock = std::make_shared<HirsClockBackendMock>();
    test_utils::ScopedClockOverride<HirsTime> guard{mock};

    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{}, NoStatus{}}));

    const auto result = HirsClock::GetInstance().Now();
    // NoStatus is an empty struct — verify it is accessible (compile + link check).
    const NoStatus status = result.Status();
    (void)status;
    SUCCEED();
}

}  // namespace time
}  // namespace score
