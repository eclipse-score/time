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
#ifndef SCORE_TIME_VEHICLE_TIME_SRC_VEHICLE_CLOCK_BACKEND_MOCK_H
#define SCORE_TIME_VEHICLE_TIME_SRC_VEHICLE_CLOCK_BACKEND_MOCK_H

#include "score/time/vehicle_time/src/vehicle_clock_backend.h"
#include "score/time/vehicle_time/src/vehicle_clock.h"

#include <gmock/gmock.h>

namespace score
{
namespace time
{

/// @brief GMock test double for the vehicle time domain.
///
/// Implements @c VehicleClockBackend so it can be injected via
/// @c test_utils::ScopedClockOverride<VehicleTime> in unit tests.
///
/// Usage:
/// @code
///   auto mock = std::make_shared<VehicleClockBackendMock>();
///   test_utils::ScopedClockOverride<VehicleTime> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
class VehicleClockBackendMock : public VehicleClockBackend
{
  public:
    VehicleClockBackendMock()                                     = default;
    ~VehicleClockBackendMock() noexcept override                  = default;
    VehicleClockBackendMock(const VehicleClockBackendMock&)              = delete;
    VehicleClockBackendMock& operator=(const VehicleClockBackendMock&)   = delete;
    VehicleClockBackendMock(VehicleClockBackendMock&&)                   = delete;
    VehicleClockBackendMock& operator=(VehicleClockBackendMock&&)        = delete;

    MOCK_METHOD((ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>),
                Now,
                (),
                (const, noexcept, override));

    MOCK_METHOD(bool, Init, (), (noexcept, override));

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

    MOCK_METHOD(void,
                SetStatusChangedCallback,
                (VehicleTime::StatusChangedCallback && callback),
                (noexcept, override));

    MOCK_METHOD(void, UnsetStatusChangedCallback, (), (noexcept, override));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_VEHICLE_CLOCK_BACKEND_MOCK_H
