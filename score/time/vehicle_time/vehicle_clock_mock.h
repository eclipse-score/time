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
#ifndef SCORE_TIME_VEHICLE_TIME_VEHICLE_CLOCK_MOCK_H
#define SCORE_TIME_VEHICLE_TIME_VEHICLE_CLOCK_MOCK_H

#include "score/time/vehicle_time/vehicle_clock_iface.h"
#include "score/time/vehicle_time/vehicle_clock.h"

#include <gmock/gmock.h>

namespace score
{
namespace time
{

/// @brief GMock test double for the vehicle time domain.
///
/// Implements @c VehicleClockIface so it can be injected via
/// @c test_utils::ScopedClockOverride<VehicleTime> in unit tests.
///
/// Usage:
/// @code
///   auto mock = std::make_shared<VehicleClockMock>();
///   test_utils::ScopedClockOverride<VehicleTime> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
class VehicleClockMock : public VehicleClockIface
{
  public:
    VehicleClockMock()                                     = default;
    ~VehicleClockMock() noexcept override                  = default;
    VehicleClockMock(const VehicleClockMock&)              = delete;
    VehicleClockMock& operator=(const VehicleClockMock&)   = delete;
    VehicleClockMock(VehicleClockMock&&)                   = delete;
    VehicleClockMock& operator=(VehicleClockMock&&)        = delete;

    MOCK_METHOD((ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>),
                Now,
                (),
                (const, noexcept, override));

    MOCK_METHOD(bool, IsAvailable, (), (const, noexcept, override));

    MOCK_METHOD(bool,
                WaitUntilAvailable,
                (const score::cpp::stop_token& token,
                 std::chrono::steady_clock::time_point until),
                (const, noexcept, override));

    MOCK_METHOD(void,
                SetTimeSlaveSyncDataReceivedCallback,
                (VehicleTime::TimeSlaveSyncDataReceivedCallback && callback),
                (noexcept, override));

    MOCK_METHOD(void, UnsetTimeSlaveSyncDataReceivedCallback, (), (noexcept, override));

    MOCK_METHOD(void,
                SetPDelayMeasurementFinishedCallback,
                (VehicleTime::PDelayMeasurementFinishedCallback && callback),
                (noexcept, override));

    MOCK_METHOD(void, UnsetPDelayMeasurementFinishedCallback, (), (noexcept, override));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_VEHICLE_CLOCK_MOCK_H
