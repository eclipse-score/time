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
#include "score/time/high_res_steady_time/src/high_res_steady_clock_backend_mock.h"
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

using SvtMock = score::td::ReceiverMock<score::td::svt::TimeBaseSnapshot>;
using SvtSnapshot = score::td::svt::TimeBaseSnapshot;
using SvtStatus = score::td::svt::TimeBaseStatus;

class VehicleClockBackendImplTest : public ::testing::Test
{
  protected:
    VehicleClockBackendImplTest()
        : mock_hirs_{std::make_shared<HighResSteadyClockBackendMock>()}
        , hirs_guard_{mock_hirs_}
        , mock_svt_{std::make_shared<SvtMock>()}
        , impl_{std::make_unique<detail::VehicleClockBackendImpl>(mock_svt_, HighResSteadyClock::GetInstance())}
    {
    }

    std::shared_ptr<HighResSteadyClockBackendMock> mock_hirs_;
    test_utils::ScopedClockOverride<HighResSteadyTime> hirs_guard_;
    std::shared_ptr<SvtMock> mock_svt_;
    std::unique_ptr<detail::VehicleClockBackendImpl> impl_;
};

TEST_F(VehicleClockBackendImplTest, IsAvailableReturnsFalseBeforeInit)
{
    EXPECT_FALSE(impl_->IsAvailable());
}

TEST_F(VehicleClockBackendImplTest, InitReturnsFalseWhenReceiverInitFails)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(false));
    EXPECT_FALSE(impl_->Init());
    EXPECT_FALSE(impl_->IsAvailable());
}

TEST_F(VehicleClockBackendImplTest, InitReturnsTrueWhenReceiverInitSucceeds)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    EXPECT_TRUE(impl_->Init());
    EXPECT_TRUE(impl_->IsAvailable());
}

TEST_F(VehicleClockBackendImplTest, InitIsIdempotentAfterSuccess)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    EXPECT_TRUE(impl_->Init());
    EXPECT_TRUE(impl_->Init());
    EXPECT_TRUE(impl_->IsAvailable());
}

TEST_F(VehicleClockBackendImplTest, InitTrueAllowsNowToReturnData)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();
    EXPECT_TRUE(impl_->IsAvailable());

    const SvtSnapshot data{1000ULL, 0ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsConsistent());
}

TEST_F(VehicleClockBackendImplTest, NowReturnsInconsistentStatusWhenNotReady)
{
    const auto snapshot = impl_->Now();
    EXPECT_FALSE(snapshot.Status().IsConsistent());
}

TEST_F(VehicleClockBackendImplTest, NowReturnsInconsistentStatusWhenReceiveReturnsNullopt)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(std::nullopt));
    const auto snapshot = impl_->Now();
    EXPECT_FALSE(snapshot.Status().IsConsistent());
}

TEST_F(VehicleClockBackendImplTest, NowReturnsInconsistentStatusWhenLocalClockBehindCapture)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{1000ULL, 600ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));

    const HighResSteadyTime::Timepoint local_now{500ns};
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{local_now, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_FALSE(snapshot.Status().IsConsistent());
}

TEST_F(VehicleClockBackendImplTest, NowComputesAdjustedTimestamp)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    // adjusted = ptp_at_capture + (now_local - local_at_capture)
    //          = 1000 + (600 - 500) = 1100 ns
    const SvtSnapshot data{1000ULL, 500ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));

    const HighResSteadyTime::Timepoint local_now{600ns};
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{local_now, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_EQ(snapshot.TimePoint().time_since_epoch().count(), 1100);
}

TEST_F(VehicleClockBackendImplTest, NowSetsSynchronizedFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 0.0, {true, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
}

TEST_F(VehicleClockBackendImplTest, NowSetsTimeOutFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, true, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));
}

TEST_F(VehicleClockBackendImplTest, NowSetsTimeLeapFutureFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, true, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeLeapFuture));
}

TEST_F(VehicleClockBackendImplTest, NowSetsTimeLeapPastFlagFromSvtStatus)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, false, true, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_TRUE(snapshot.Status().IsFlagActive(VehicleTime::StatusFlag::kTimeLeapPast));
}

TEST_F(VehicleClockBackendImplTest, NowReturnsInconsistentStatusWhenIsCorrectIsFalse)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 0.0, {false, false, false, false, false}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_FALSE(snapshot.Status().IsConsistent());
}

TEST_F(VehicleClockBackendImplTest, NowForwardsRateDeviation)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();

    const SvtSnapshot data{0ULL, 0ULL, 2.5, {false, false, false, false, true}, {}, {}};
    EXPECT_CALL(*mock_svt_, Receive()).WillOnce(Return(data));
    EXPECT_CALL(*mock_hirs_, Now())
        .WillOnce(Return(ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{0ns}, NoStatus{}}));

    const auto snapshot = impl_->Now();
    EXPECT_DOUBLE_EQ(snapshot.Status().RateDeviation(), 2.5);
}

TEST_F(VehicleClockBackendImplTest, WaitUntilAvailableReturnsTrueWhenInitSucceeds)
{
    EXPECT_CALL(*mock_svt_, Init()).WillOnce(Return(true));
    impl_->Init();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
    EXPECT_TRUE(impl_->WaitUntilAvailable(score::cpp::stop_source{}.get_token(), deadline));
}

TEST_F(VehicleClockBackendImplTest, WaitUntilAvailableReturnsFalseWhenStopRequested)
{
    score::cpp::stop_source ss;
    ss.request_stop();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
    EXPECT_FALSE(impl_->WaitUntilAvailable(ss.get_token(), deadline));
}

TEST_F(VehicleClockBackendImplTest, WaitUntilAvailableReturnsFalseWhenDeadlinePassed)
{
    const auto past_deadline = std::chrono::steady_clock::now() - std::chrono::seconds{1};
    EXPECT_FALSE(impl_->WaitUntilAvailable(score::cpp::stop_source{}.get_token(), past_deadline));
}

TEST_F(VehicleClockBackendImplTest, CallbackMethodsAreNoOps)
{
    impl_->SetTimeSlaveSyncDataReceivedCallback(
        [](const TimeSlaveSyncData<VehicleTime>&) {});
    impl_->UnsetTimeSlaveSyncDataReceivedCallback();
    impl_->SetPDelayMeasurementFinishedCallback(
        [](const PDelayMeasurementData<VehicleTime>&) {});
    impl_->UnsetPDelayMeasurementFinishedCallback();
    impl_->SetStatusChangedCallback([](const VehicleTimeStatus&) {});
    impl_->UnsetStatusChangedCallback();
}

}  // namespace
}  // namespace time
}  // namespace score
