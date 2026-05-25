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
#include "examples/time/vehicle_time/vehicle_time_handler.h"

#include "score/time/vehicle_time/vehicle_clock_mock.h"
#include "score/time/hpls_time/hpls_clock_mock.h"
#include "score/time/clock/scoped_clock_override.h"
#include "score/time/clock/clock_snapshot.h"

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

/// @brief Test fixture: replaces both VehicleClock and HplsClock with mocks.
///
/// This is the key pattern for components that depend on multiple time bases:
/// each clock domain has its own ScopedClockOverride that injects the mock
/// backend for the duration of the test.
class VehicleTimeHandlerTest : public ::testing::Test
{
  protected:
    VehicleTimeHandlerTest()
        : vehicle_mock_{std::make_shared<score::time::VehicleClockMock>()}
        , hpls_mock_{std::make_shared<score::time::HplsClockMock>()}
        , vehicle_guard_{vehicle_mock_}
        , hpls_guard_{hpls_mock_}
    {
    }

    std::shared_ptr<score::time::VehicleClockMock> vehicle_mock_;
    std::shared_ptr<score::time::HplsClockMock>    hpls_mock_;
    score::time::test_utils::ScopedClockOverride<score::time::VehicleTime>  vehicle_guard_;
    score::time::test_utils::ScopedClockOverride<score::time::HplsTime>     hpls_guard_;
};

TEST_F(VehicleTimeHandlerTest, ReportContainsSynchronizedVehicleTimeAndHplsTime)
{
    // Prepare a synchronized vehicle time snapshot.
    score::time::VehicleTimeStatus status;
    status.flags = score::time::ClockStatus<score::time::VehicleTime::StatusFlag>{
        {score::time::VehicleTime::StatusFlag::kSynchronized}};
    status.rate_deviation = 1.5e-9;

    const score::time::VehicleTime::Timepoint vehicle_tp{std::chrono::nanoseconds{5'000'000'000LL}};
    const score::time::ClockSnapshot<score::time::VehicleTime::Timepoint, score::time::VehicleTimeStatus>
        vehicle_snapshot{vehicle_tp, status};

    const score::time::HplsTime::Timepoint hpls_tp{std::chrono::nanoseconds{1'234'567'890LL}};
    const score::time::ClockSnapshot<score::time::HplsTime::Timepoint, score::time::NoStatus>
        hpls_snapshot{hpls_tp, score::time::NoStatus{}};

    EXPECT_CALL(*vehicle_mock_, Now()).WillOnce(Return(vehicle_snapshot));
    EXPECT_CALL(*hpls_mock_,    Now()).WillOnce(Return(hpls_snapshot));

    VehicleTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.vehicle_time_ns, 5'000'000'000LL);
    EXPECT_EQ(report.hpls_time_ns,    1'234'567'890LL);
    EXPECT_TRUE(report.synchronized);
    EXPECT_TRUE(report.valid);
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
    EXPECT_CALL(*hpls_mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HplsTime::Timepoint, score::time::NoStatus>{
            score::time::HplsTime::Timepoint{}, score::time::NoStatus{}}));

    VehicleTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_FALSE(report.synchronized);
    EXPECT_EQ(report.vehicle_time_ns, 0LL);
}

TEST_F(VehicleTimeHandlerTest, HplsTimeIsIndependentFromVehicleTimeSynchronization)
{
    // Even when vehicle time is not synchronized, hpls_time_ns is still populated
    // from the local monotonic clock.
    score::time::VehicleTimeStatus unsync_status;

    const score::time::HplsTime::Timepoint hpls_tp{std::chrono::nanoseconds{99'000'000LL}};

    EXPECT_CALL(*vehicle_mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::VehicleTime::Timepoint, score::time::VehicleTimeStatus>{
            score::time::VehicleTime::Timepoint{}, unsync_status}));
    EXPECT_CALL(*hpls_mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HplsTime::Timepoint, score::time::NoStatus>{
            hpls_tp, score::time::NoStatus{}}));

    VehicleTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.hpls_time_ns, 99'000'000LL);
    EXPECT_FALSE(report.synchronized);
}

}  // namespace test
}  // namespace vehicle_time
}  // namespace time
}  // namespace examples
