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
#ifndef SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_STUB_IMPL_VEHICLE_CLOCK_BACKEND_IMPL_H
#define SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_STUB_IMPL_VEHICLE_CLOCK_BACKEND_IMPL_H

// Internal header — include ONLY from stub_impl/vehicle_clock_backend_impl.cpp.

#include "score/time/vehicle_time/src/vehicle_clock_backend.h"

#include <score/stop_token.hpp>

#include <chrono>

namespace score
{
namespace time
{
namespace detail
{

/// @brief Stub backend for the vehicle time domain (host/test-only).
///
/// Returns safe zero/false defaults for all methods.
class VehicleClockBackendImpl final : public VehicleClockBackend
{
  public:
    VehicleClockBackendImpl() noexcept                            = default;
    ~VehicleClockBackendImpl() noexcept override                  = default;
    VehicleClockBackendImpl(const VehicleClockBackendImpl&)              = delete;
    VehicleClockBackendImpl& operator=(const VehicleClockBackendImpl&)   = delete;
    VehicleClockBackendImpl(VehicleClockBackendImpl&&)                   = delete;
    VehicleClockBackendImpl& operator=(VehicleClockBackendImpl&&)        = delete;

    ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus> Now() const noexcept override
    {
        return ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>{};
    }

    bool Init() noexcept override
    {
        return true;
    }

    bool IsAvailable() const noexcept override
    {
        return false;
    }

    bool WaitUntilAvailable(const score::cpp::stop_token& /*token*/,
                            std::chrono::steady_clock::time_point /*until*/) const noexcept override
    {
        return false;
    }

    void SetTimeSlaveSyncDataReceivedCallback(
        VehicleTime::TimeSlaveSyncDataReceivedCallback&& /*callback*/) noexcept override
    {
    }

    void UnsetTimeSlaveSyncDataReceivedCallback() noexcept override {}

    void SetPDelayMeasurementFinishedCallback(
        VehicleTime::PDelayMeasurementFinishedCallback&& /*callback*/) noexcept override
    {
    }

    void UnsetPDelayMeasurementFinishedCallback() noexcept override {}

    void SetStatusChangedCallback(
        VehicleTime::StatusChangedCallback&& /*callback*/) noexcept override
    {
    }

    void UnsetStatusChangedCallback() noexcept override {}
};

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_STUB_IMPL_VEHICLE_CLOCK_BACKEND_IMPL_H
