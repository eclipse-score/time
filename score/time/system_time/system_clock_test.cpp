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
#include "score/time/system_time/system_clock_mock.h"
#include "score/time/clock/scoped_clock_override.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <type_traits>

using ::testing::Return;

namespace score
{
namespace time
{

namespace
{

class SampleSystemService
{
  public:
    [[nodiscard]] std::chrono::system_clock::time_point GetCurrentTime() const noexcept
    {
        return SystemClock::GetInstance().Now().TimePoint();
    }
};

}  // namespace

TEST(SystemClockTest, NowReturnsTimepointSuitableForDurationArithmetic)
{
    auto mock = std::make_shared<SystemClockMock>();
    test_utils::ScopedClockOverride<std::chrono::system_clock> guard{mock};

    const std::chrono::system_clock::time_point tp{std::chrono::nanoseconds{1'000'000LL}};
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<std::chrono::system_clock::time_point, NoStatus>{tp, NoStatus{}}));

    const auto result   = SystemClock::GetInstance().Now();
    const auto deadline = result.TimePoint() + std::chrono::seconds{5};

    EXPECT_EQ(deadline.time_since_epoch(),
              std::chrono::nanoseconds{1'000'000LL} + std::chrono::seconds{5});
}

TEST(SystemClockTest, NowReturnsExactTimepointFromMock)
{
    auto mock = std::make_shared<SystemClockMock>();
    test_utils::ScopedClockOverride<std::chrono::system_clock> guard{mock};

    const std::chrono::system_clock::time_point tp{std::chrono::seconds{42}};
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<std::chrono::system_clock::time_point, NoStatus>{tp, NoStatus{}}));

    EXPECT_EQ(SystemClock::GetInstance().Now().TimePoint(), tp);
}

TEST(SystemClockTest, NowSnapshotCarriesNoStatus)
{
    auto mock = std::make_shared<SystemClockMock>();
    test_utils::ScopedClockOverride<std::chrono::system_clock> guard{mock};

    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<std::chrono::system_clock::time_point, NoStatus>{
            std::chrono::system_clock::time_point{}, NoStatus{}}));

    const auto result   = SystemClock::GetInstance().Now();
    const NoStatus status = result.Status();
    (void)status;
    SUCCEED();
}

TEST(SystemClockTest, ScopedClockOverrideInjectsMockIntoSut)
{
    auto mock = std::make_shared<SystemClockMock>();
    test_utils::ScopedClockOverride<std::chrono::system_clock> guard{mock};

    const std::chrono::system_clock::time_point expected{std::chrono::nanoseconds{999LL}};
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<std::chrono::system_clock::time_point, NoStatus>{expected, NoStatus{}}));

    SampleSystemService sut;
    EXPECT_EQ(sut.GetCurrentTime(), expected);
}

TEST(SystemClockTest, ScopedClockOverrideRestoresBackendAfterScope)
{
    auto mock = std::make_shared<SystemClockMock>();
    {
        test_utils::ScopedClockOverride<std::chrono::system_clock> guard{mock};
        const std::chrono::system_clock::time_point tp{std::chrono::seconds{1}};
        EXPECT_CALL(*mock, Now()).WillOnce(Return(
            ClockSnapshot<std::chrono::system_clock::time_point, NoStatus>{tp, NoStatus{}}));
        EXPECT_EQ(SystemClock::GetInstance().Now().TimePoint(), tp);
    }
    // After guard goes out of scope, a new guard must succeed without assertion.
    auto mock2 = std::make_shared<SystemClockMock>();
    test_utils::ScopedClockOverride<std::chrono::system_clock> guard2{mock2};
    const std::chrono::system_clock::time_point tp2{std::chrono::seconds{2}};
    EXPECT_CALL(*mock2, Now()).WillOnce(Return(
        ClockSnapshot<std::chrono::system_clock::time_point, NoStatus>{tp2, NoStatus{}}));
    EXPECT_EQ(SystemClock::GetInstance().Now().TimePoint(), tp2);
}

}  // namespace time
}  // namespace score
