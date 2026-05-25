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
#include "score/time/vehicle_time/vehicle_clock_mock.h"
#include "score/time/clock/scoped_clock_override.h"

#include <score/stop_token.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <type_traits>

using ::testing::_;
using ::testing::Return;

namespace score
{
namespace time
{

namespace
{

class SampleVehicleService
{
  public:
    [[nodiscard]] bool CheckAvailable() const noexcept
    {
        return VehicleClock::GetInstance().IsAvailable();
    }

    [[nodiscard]] VehicleTime::Timepoint GetCurrentTime() const noexcept
    {
        return VehicleClock::GetInstance().Now().TimePoint();
    }
};

}  // namespace

TEST(VehicleClockTest, NowReturnsSynchronizedStatusAndTimepoint)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    VehicleTimeStatus status;
    status.flags = ClockStatus<VehicleTime::StatusFlag>{{VehicleTime::StatusFlag::kSynchronized}};
    const ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus> snapshot{
        VehicleTime::Timepoint{std::chrono::nanoseconds{42LL}}, status};

    EXPECT_CALL(*mock, Now()).WillOnce(Return(snapshot));

    const auto result = VehicleClock::GetInstance().Now();

    EXPECT_TRUE(result.Status().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
    EXPECT_EQ(result.TimePoint().time_since_epoch(), std::chrono::nanoseconds{42LL});
}

TEST(VehicleClockTest, NowIsSynchronizedReturnsTrueWhenFlagSet)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    VehicleTimeStatus status;
    status.flags = ClockStatus<VehicleTime::StatusFlag>{{VehicleTime::StatusFlag::kSynchronized}};
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>{VehicleTime::Timepoint{}, status}));

    EXPECT_TRUE(VehicleClock::GetInstance().Now().Status().IsSynchronized());
}

TEST(VehicleClockTest, NowIsSynchronizedReturnsFalseWhenTimeoutSet)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    VehicleTimeStatus status;
    status.flags = ClockStatus<VehicleTime::StatusFlag>{
        {VehicleTime::StatusFlag::kSynchronized, VehicleTime::StatusFlag::kTimeOut}};
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>{VehicleTime::Timepoint{}, status}));

    EXPECT_FALSE(VehicleClock::GetInstance().Now().Status().IsSynchronized());
}

// ── UC2: IsAvailable / WaitUntilAvailable ─────────────────────────────────────

TEST(VehicleClockTest, IsAvailableReturnsTrueWhenBackendReports)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    EXPECT_CALL(*mock, IsAvailable()).WillOnce(Return(true));

    EXPECT_TRUE(VehicleClock::GetInstance().IsAvailable());
}

TEST(VehicleClockTest, IsAvailableReturnsFalseWhenBackendUnavailable)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    EXPECT_CALL(*mock, IsAvailable()).WillOnce(Return(false));

    EXPECT_FALSE(VehicleClock::GetInstance().IsAvailable());
}

TEST(VehicleClockTest, WaitUntilAvailableForwardsTokenAndDeadlineToBackend)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    score::cpp::stop_source source;
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds{100};

    EXPECT_CALL(*mock, WaitUntilAvailable(_, _)).WillOnce(Return(true));

    EXPECT_TRUE(VehicleClock::GetInstance().WaitUntilAvailable(source.get_token(), until));
}

// ── UC3: Subscribe / Unsubscribe — callback is captured and invocable ─────────

TEST(VehicleClockTest, SubscribeTimeSlaveSyncDataCapturesAndInvokesCallback)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    VehicleTime::TimeSlaveSyncDataReceivedCallback captured_cb;
    EXPECT_CALL(*mock, SetTimeSlaveSyncDataReceivedCallback(_))
        .WillOnce([&captured_cb](VehicleTime::TimeSlaveSyncDataReceivedCallback&& cb) {
            captured_cb = std::move(cb);
        });

    bool invoked{false};
    VehicleClock::GetInstance().Subscribe<TimeSlaveSyncData<VehicleTime>>(
        [&invoked](const TimeSlaveSyncData<VehicleTime>&) { invoked = true; });

    TimeSlaveSyncData<VehicleTime> data{};
    captured_cb(data);

    EXPECT_TRUE(invoked);
}

TEST(VehicleClockTest, UnsubscribeTimeSlaveSyncDataForwardsToBackend)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    EXPECT_CALL(*mock, UnsetTimeSlaveSyncDataReceivedCallback()).Times(1);

    VehicleClock::GetInstance().Unsubscribe<TimeSlaveSyncData<VehicleTime>>();
}

// ── UC4: StatusFlag type alias is accessible via VehicleClock ─────────────────

TEST(VehicleClockTest, VehicleTimeStatusFlagValues)
{
    EXPECT_EQ(static_cast<std::uint8_t>(VehicleTime::StatusFlag::kSynchronized), 1U);
    EXPECT_EQ(static_cast<std::uint8_t>(VehicleTime::StatusFlag::kTimeOut), 0U);
    EXPECT_EQ(static_cast<std::uint8_t>(VehicleTime::StatusFlag::kTimeLeapFuture), 3U);
}

// ── UC6: ScopedClockOverride injects mock into a SUT that calls GetInstance() ──

TEST(VehicleClockTest, ScopedClockOverrideInjectsMockIntoSut)
{
    auto mock = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard{mock};

    const VehicleTime::Timepoint expected_tp{std::chrono::nanoseconds{999LL}};

    EXPECT_CALL(*mock, IsAvailable()).WillOnce(Return(true));
    EXPECT_CALL(*mock, Now()).WillOnce(Return(
        ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>{expected_tp, VehicleTimeStatus{}}));

    SampleVehicleService sut;
    EXPECT_TRUE(sut.CheckAvailable());
    EXPECT_EQ(sut.GetCurrentTime(), expected_tp);
}

TEST(VehicleClockTest, ScopedClockOverrideRestoresBackendAfterScope)
{
    auto mock = std::make_shared<VehicleClockMock>();
    {
        test_utils::ScopedClockOverride<VehicleTime> guard{mock};
        EXPECT_CALL(*mock, IsAvailable()).WillOnce(Return(false));
        EXPECT_FALSE(VehicleClock::GetInstance().IsAvailable());
    }
    // After guard goes out of scope, override is cleared.
    // A new guard for the same Tag must succeed without assertion.
    auto mock2 = std::make_shared<VehicleClockMock>();
    test_utils::ScopedClockOverride<VehicleTime> guard2{mock2};
    EXPECT_CALL(*mock2, IsAvailable()).WillOnce(Return(true));
    EXPECT_TRUE(VehicleClock::GetInstance().IsAvailable());
}

}  // namespace time
}  // namespace score
