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
#include "examples/time/vehicle_time/src/vehicle_time_handler.h"

#include "score/time/vehicle_time/src/vehicle_clock_backend_mock.h"
#include "score/time/high_res_steady_time/src/high_res_steady_clock_backend_mock.h"
#include "score/time/clock/src/scoped_clock_override.h"
#include "score/time/clock/src/clock_snapshot.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

using ::testing::Return;

namespace examples
{
namespace time
{
namespace vehicle_time
{
namespace test
{

/// @brief Test fixture: replaces both VehicleClock and HighResSteadyClock with mocks.
///
/// This is the key pattern for components that depend on multiple time bases:
/// each clock domain has its own ScopedClockOverride that injects the mock
/// backend for the duration of the test.
class VehicleTimeHandlerTest : public ::testing::Test
{
  protected:
    VehicleTimeHandlerTest()
        : vehicle_mock_{std::make_shared<score::time::VehicleClockBackendMock>()}
        , hirs_mock_{std::make_shared<score::time::HighResSteadyClockBackendMock>()}
        , vehicle_guard_{vehicle_mock_}
        , hirs_guard_{hirs_mock_}
    {
    }

    std::shared_ptr<score::time::VehicleClockBackendMock> vehicle_mock_;
    std::shared_ptr<score::time::HighResSteadyClockBackendMock> hirs_mock_;
    score::time::test_utils::ScopedClockOverride<score::time::VehicleTime> vehicle_guard_;
    score::time::test_utils::ScopedClockOverride<score::time::HighResSteadyTime> hirs_guard_;
};

TEST_F(VehicleTimeHandlerTest, InitReturnsTrueWhenBackendInitSucceeds)
{
    EXPECT_CALL(*vehicle_mock_, Init()).WillOnce(Return(true));
    VehicleTimeHandler handler;
    EXPECT_TRUE(handler.Init());
}

TEST_F(VehicleTimeHandlerTest, InitReturnsFalseWhenBackendInitFails)
{
    EXPECT_CALL(*vehicle_mock_, Init()).WillOnce(Return(false));
    VehicleTimeHandler handler;
    EXPECT_FALSE(handler.Init());
}

TEST_F(VehicleTimeHandlerTest, ReportContainsSynchronizedVehicleTimeAndHighResSteadyTime)
{
    score::time::VehicleTimeStatus status;
    status.flags = score::time::ClockStatus<score::time::VehicleTime::StatusFlag>{
        {score::time::VehicleTime::StatusFlag::kSynchronized}};
    status.rate_deviation = 1.5e-9;

    const score::time::VehicleTime::Timepoint vehicle_tp{std::chrono::nanoseconds{5'000'000'000LL}};
    const score::time::ClockSnapshot<score::time::VehicleTime::Timepoint, score::time::VehicleTimeStatus>
        vehicle_snapshot{vehicle_tp, status};

    const score::time::HighResSteadyTime::Timepoint hirs_tp{std::chrono::nanoseconds{1'234'567'890LL}};
    const score::time::ClockSnapshot<score::time::HighResSteadyTime::Timepoint, score::time::NoStatus>
        hirs_snapshot{hirs_tp, score::time::NoStatus{}};

    EXPECT_CALL(*vehicle_mock_, Now()).WillOnce(Return(vehicle_snapshot));
    EXPECT_CALL(*hirs_mock_,    Now()).WillOnce(Return(hirs_snapshot));

    VehicleTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.vehicle_time_ns, 5'000'000'000LL);
    EXPECT_EQ(report.high_res_steady_time_ns,    1'234'567'890LL);
    EXPECT_TRUE(report.is_reliable);
    EXPECT_TRUE(report.is_consistent);
    EXPECT_DOUBLE_EQ(report.rate_deviation, 1.5e-9);
}

TEST_F(VehicleTimeHandlerTest, ReportShowsNotSynchronizedWhenTimeOutFlagIsSet)
{
    score::time::VehicleTimeStatus status;
    status.flags = score::time::ClockStatus<score::time::VehicleTime::StatusFlag>{
        {score::time::VehicleTime::StatusFlag::kTimeOut}};

    EXPECT_CALL(*vehicle_mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::VehicleTime::Timepoint, score::time::VehicleTimeStatus>{
            score::time::VehicleTime::Timepoint{}, status}));
    EXPECT_CALL(*hirs_mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HighResSteadyTime::Timepoint, score::time::NoStatus>{
            score::time::HighResSteadyTime::Timepoint{}, score::time::NoStatus{}}));

    VehicleTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_FALSE(report.is_reliable);
    EXPECT_EQ(report.vehicle_time_ns, 0LL);
}

TEST_F(VehicleTimeHandlerTest, HighResSteadyTimeIsIndependentFromVehicleTimeSynchronization)
{
    // Even when vehicle time is not synchronized, high_res_steady_time_ns is still populated
    // from the local monotonic clock.
    score::time::VehicleTimeStatus unsync_status;

    const score::time::HighResSteadyTime::Timepoint hirs_tp{std::chrono::nanoseconds{99'000'000LL}};

    EXPECT_CALL(*vehicle_mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::VehicleTime::Timepoint, score::time::VehicleTimeStatus>{
            score::time::VehicleTime::Timepoint{}, unsync_status}));
    EXPECT_CALL(*hirs_mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HighResSteadyTime::Timepoint, score::time::NoStatus>{
            hirs_tp, score::time::NoStatus{}}));

    VehicleTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.high_res_steady_time_ns, 99'000'000LL);
    EXPECT_FALSE(report.is_reliable);
}

TEST_F(VehicleTimeHandlerTest, RegisterStatusCallbackForwardsToBackend)
{
    EXPECT_CALL(*vehicle_mock_,
                SetStatusChangedCallback(::testing::_))
        .Times(1);

    VehicleTimeHandler handler;
    handler.RegisterStatusCallback([](const score::time::VehicleTimeStatus&) {});
}

TEST_F(VehicleTimeHandlerTest, UnregisterStatusCallbackForwardsToBackend)
{
    EXPECT_CALL(*vehicle_mock_, UnsetStatusChangedCallback()).Times(1);

    VehicleTimeHandler handler;
    handler.UnregisterStatusCallback();
}

}  // namespace test
}  // namespace vehicle_time
}  // namespace time
}  // namespace examples
