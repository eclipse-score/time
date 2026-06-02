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
#include "score/time/vehicle_time/src/details/td_impl/vehicle_clock_backend_impl.h"

#include "score/time_daemon/src/ipc/receiver_mock.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"
#include "score/time/hirs_time/src/hirs_clock_backend_mock.h"
#include "score/time/clock/src/scoped_clock_override.h"
#include "score/time/clock/src/clock_snapshot.h"
#include "score/time/clock/src/no_status.h"

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
        : mock_hirs_{std::make_shared<HirsClockBackendMock>()}
        , hirs_guard_{mock_hirs_}
        , mock_svt_{std::make_shared<SvtMock>()}
        , impl_{std::make_unique<detail::VehicleClockBackendImpl>(mock_svt_, HirsClock::GetInstance())}
    {
    }

    std::shared_ptr<HirsClockBackendMock>             mock_hirs_;
    test_utils::ScopedClockOverride<HirsTime>              hirs_guard_;
    std::shared_ptr<SvtMock>                  mock_svt_;
    std::unique_ptr<detail::VehicleClockBackendImpl> impl_;
};

// ── Init / IsAvailable ───────────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, IsAvailableReturnsFalseBeforeInit)
{
    EXPECT_FALSE(impl_->IsAvailable());
}

TEST_F(VehicleTimeImplTest, InitReturnsFalseWhenReceiverInitFails)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(false));
    EXPECT_FALSE(impl_->Init());
    EXPECT_FALSE(impl_->IsAvailable());
}

TEST_F(VehicleTimeImplTest, InitReturnsTrueWhenReceiverInitSucceeds)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    EXPECT_TRUE(impl_->Init());
    EXPECT_TRUE(impl_->IsAvailable());
}

TEST_F(VehicleTimeImplTest, InitIsIdempotentAfterSuccess)
{
    // svt_receiver_->Init() is called only once even when Init() is called twice.
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    EXPECT_TRUE(impl_->Init());
    EXPECT_TRUE(impl_->Init());  // idempotent — no second svt_receiver_->Init()
    EXPECT_TRUE(impl_->IsAvailable());
}

TEST_F(VehicleTimeImplTest, InitTrueAllowsNowToReturnData)
{
    // Control-flow: Init() → IsAvailable() == true → Now() returns non-kUnknown snapshot.
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();
    EXPECT_TRUE(impl_->IsAvailable());

    const SvtSnapshot data{1000ULL, 0ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_FALSE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

// ── Now — pre-conditions ──────────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, NowReturnsUnknownStatusWhenNotReady)
{
    // is_ready_ is false by default — Init() not called
    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

TEST_F(VehicleTimeImplTest, NowReturnsUnknownStatusWhenReceiveReturnsNullopt)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(std::nullopt));
    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

TEST_F(VehicleTimeImplTest, NowReturnsUnknownStatusWhenLocalClockBehindCapture)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    // local_time = 600, now_local = 500 → now_local < local_at_capture → unknown
    const SvtSnapshot data{1000ULL, 600ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));

    const HirsTime::Timepoint local_now{500ns};
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{local_now, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

// ── Now — adjusted timestamp ──────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, NowComputesAdjustedTimestamp)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    // adjusted = ptp_at_capture + (now_local - local_at_capture)
    //          = 1000 + (600 - 500) = 1100 ns
    const SvtSnapshot data{1000ULL, 500ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));

    const HirsTime::Timepoint local_now{600ns};
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{local_now, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_EQ(snapshot.TimePoint().time_since_epoch().count(), 1100);
}

// ── Now — ConvertPtpStatus mapping ────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, NowSetsSynchronizedFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
}

TEST_F(VehicleTimeImplTest, NowSetsTimeOutFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    // is_timeout = true
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, true, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));
}

TEST_F(VehicleTimeImplTest, NowSetsTimeLeapFutureFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    // is_time_jump_future = true
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, true, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeLeapFuture));
}

TEST_F(VehicleTimeImplTest, NowSetsTimeLeapPastFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    // is_time_jump_past = true
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, false, true, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeLeapPast));
}

TEST_F(VehicleTimeImplTest, NowSetsUnknownFlagWhenIsCorrectIsFalse)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    // is_correct = false → kUnknown
    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, false, false, false}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kUnknown));
}

TEST_F(VehicleTimeImplTest, NowForwardsRateDeviation)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 2.5, {false, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HirsTime::Timepoint, NoStatus>{HirsTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_DOUBLE_EQ(snapshot.Status().RateDeviation(), 2.5);
}

// ── WaitUntilAvailable ───────────────────────────────────────────────────────

TEST_F(VehicleTimeImplTest, WaitUntilAvailableReturnsTrueWhenInitSucceeds)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
    EXPECT_TRUE(impl_->WaitUntilAvailable(score::cpp::stop_source{}.get_token(), deadline));
}

TEST_F(VehicleTimeImplTest, WaitUntilAvailableReturnsFalseWhenStopRequested)
{
    score::cpp::stop_source ss;
    ss.request_stop();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
    EXPECT_FALSE(impl_->WaitUntilAvailable(ss.get_token(), deadline));
}

TEST_F(VehicleTimeImplTest, WaitUntilAvailableReturnsFalseWhenDeadlinePassed)
{
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
