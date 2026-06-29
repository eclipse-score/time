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
#include "score/time/high_res_steady_time/src/high_res_steady_clock_backend_mock.h"
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

TEST(HighResSteadyClockTest, NowReturnsTimepointSuitableForDurationArithmetic)
{
    auto mock = std::make_shared<HighResSteadyClockBackendMock>();
    test_utils::ScopedClockOverride<HighResSteadyTime> guard{mock};

    const HighResSteadyTime::Timepoint tp{std::chrono::nanoseconds{1'000'000LL}};
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{tp, NoStatus{}}));

    const auto result = HighResSteadyClock::GetInstance().Now();
    const auto deadline = result.TimePoint() + std::chrono::seconds{3};

    EXPECT_EQ(deadline.time_since_epoch(),
              std::chrono::nanoseconds{1'000'000LL} + std::chrono::seconds{3});
}

TEST(HighResSteadyClockTest, NowReturnsZeroTimepointByDefault)
{
    auto mock = std::make_shared<HighResSteadyClockBackendMock>();
    test_utils::ScopedClockOverride<HighResSteadyTime> guard{mock};

    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{}, NoStatus{}}));

    const auto result = HighResSteadyClock::GetInstance().Now();

    EXPECT_EQ(result.TimePoint().time_since_epoch(), std::chrono::nanoseconds{0});
}

TEST(HighResSteadyClockTest, NowSnapshotCarriesNoStatus)
{
    auto mock = std::make_shared<HighResSteadyClockBackendMock>();
    test_utils::ScopedClockOverride<HighResSteadyTime> guard{mock};

    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{}, NoStatus{}}));

    const auto result = HighResSteadyClock::GetInstance().Now();
    // NoStatus is an empty struct — verify it is accessible (compile + link check).
    const NoStatus status = result.Status();
    (void)status;
    SUCCEED();
}

}  // namespace time
}  // namespace score
