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
#include "score/time/vehicle_time/details/td_impl/vehicle_clock_impl.h"

#include "score/TimeDaemon/code/ipc/receiver_mock.h"
#include "score/TimeDaemon/code/ipc/svt/svt_time_info.h"
#include "score/time/hpls_time/hpls_clock_mock.h"
#include "score/time/clock/scoped_clock_override.h"
#include "score/time/clock/clock_snapshot.h"
#include "score/time/clock/no_status.h"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>

namespace score
{
namespace time
{
namespace
{

using namespace std::chrono_literals;
using ::testing::Return;

using SvtMock     = score::td::ReceiverMock<score::td::svt::TimeBaseSnapshot>;
using SvtSnapshot = score::td::svt::TimeBaseSnapshot;
using SvtStatus   = score::td::svt::TimeBaseStatus;

class VehicleTimeImplTest : public ::testing::Test
{
  protected:
    VehicleTimeImplTest()
        : mock_hpls_{std::make_shared<HplsClockMock>()}
        , hpls_guard_{mock_hpls_}
        , mock_svt_{std::make_shared<SvtMock>()}
        , impl_{std::make_unique<detail::VehicleClockImpl>(mock_svt_, HplsClock::GetInstance())}
    {
    }

    std::shared_ptr<HplsClockMock>             mock_hpls_;
    test_utils::ScopedClockOverride<HplsTime>              hpls_guard_;
    std::shared_ptr<SvtMock>                  mock_svt_;
    std::unique_ptr<detail::VehicleClockImpl> impl_;
};

// ── IsAvailable ──────────────────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, IsAvailableReturnsFalseWhenInitFails)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(false));
    EXPECT_FALSE(impl_->IsAvailable());
}

TEST_F(VehicleTimeImplTest, IsAvailableReturnsTrueAfterSuccessfulInit)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    EXPECT_TRUE(impl_->IsAvailable());
}

TEST_F(VehicleTimeImplTest, IsAvailableCachesResultAfterSuccess)
{
    // Init() is called only once even though IsAvailable() is queried twice.
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    EXPECT_TRUE(impl_->IsAvailable());
    EXPECT_TRUE(impl_->IsAvailable());
}

// ── Now — pre-conditions ──────────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, NowReturnsUnknownStatusWhenNotReady)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(false));
    impl_->IsAvailable();

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

TEST_F(VehicleTimeImplTest, NowReturnsUnknownStatusWhenReceiveReturnsNullopt)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(std::nullopt));
    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

TEST_F(VehicleTimeImplTest, NowReturnsUnknownStatusWhenLocalClockBehindCapture)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    // local_time = 600, now_local = 500 → now_local < local_at_capture → unknown
    const SvtSnapshot data{1000ULL, 600ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));

    const HplsTime::Timepoint local_now{500ns};
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{local_now, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

// ── Now — adjusted timestamp ──────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, NowComputesAdjustedTimestamp)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    // adjusted = ptp_at_capture + (now_local - local_at_capture)
    //          = 1000 + (600 - 500) = 1100 ns
    const SvtSnapshot data{1000ULL, 500ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));

    const HplsTime::Timepoint local_now{600ns};
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{local_now, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_EQ(snapshot.TimePoint().time_since_epoch().count(), 1100);
}

// ── Now — ConvertPtpStatus mapping ────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, NowSetsSynchronizedFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    const SvtSnapshot data{0ULL, 0ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{HplsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
}

TEST_F(VehicleTimeImplTest, NowSetsTimeOutFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    // is_timeout = true
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, true, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{HplsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));
}

TEST_F(VehicleTimeImplTest, NowSetsTimeLeapFutureFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    // is_time_jump_future = true
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, true, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{HplsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeLeapFuture));
}

TEST_F(VehicleTimeImplTest, NowSetsTimeLeapPastFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    // is_time_jump_past = true
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, false, true, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{HplsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeLeapPast));
}

TEST_F(VehicleTimeImplTest, NowSetsUnknownFlagWhenIsCorrectIsFalse)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    // is_correct = false → kUnknown
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, false, false, false}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{HplsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

TEST_F(VehicleTimeImplTest, NowForwardsRateDeviation)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->IsAvailable();

    const SvtSnapshot data{0ULL, 0ULL, 2.5, {false, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hpls_, Now())
        .WillOnce(Return(ClockSnapshot<HplsTime::Timepoint, NoStatus>{HplsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_DOUBLE_EQ(snapshot.Status().rate_deviation, 2.5);
}

// ── WaitUntilAvailable ───────────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, WaitUntilAvailableReturnsTrueWhenInitSucceeds)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
    EXPECT_TRUE(impl_->WaitUntilAvailable(score::cpp::stop_source{}.get_token(), deadline));
}

TEST_F(VehicleTimeImplTest, WaitUntilAvailableReturnsFalseWhenStopRequested)
{
    EXPECT_CALL(*mock_svt_, Init()).WillRepeatedly(Return(false));
    score::cpp::stop_source ss;
    ss.request_stop();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
    EXPECT_FALSE(impl_->WaitUntilAvailable(ss.get_token(), deadline));
}

TEST_F(VehicleTimeImplTest, WaitUntilAvailableReturnsFalseWhenDeadlinePassed)
{
    EXPECT_CALL(*mock_svt_, Init()).WillRepeatedly(Return(false));
    const auto past_deadline = std::chrono::steady_clock::now() - std::chrono::seconds{1};
    EXPECT_FALSE(impl_->WaitUntilAvailable(score::cpp::stop_source{}.get_token(), past_deadline));
}

// ── Callback no-ops ──────────────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, CallbackMethodsAreNoOps)
{
    impl_->SetTimeSlaveSyncDataReceivedCallback(
        [](const TimeSlaveSyncData<VehicleTime>&) {});
    impl_->UnsetTimeSlaveSyncDataReceivedCallback();
    impl_->SetPDelayMeasurementFinishedCallback(
        [](const PDelayMeasurementData<VehicleTime>&) {});
    impl_->UnsetPDelayMeasurementFinishedCallback();
    // No crash = pass.
}

}  // namespace
}  // namespace time
}  // namespace score
