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
#ifndef SCORE_TIME_VEHICLE_TIME_SRC_VEHICLE_CLOCK_BACKEND_H
#define SCORE_TIME_VEHICLE_TIME_SRC_VEHICLE_CLOCK_BACKEND_H

#include "score/time/vehicle_time/src/vehicle_time.h"
#include "score/time/clock/src/clock_snapshot.h"

#include <score/stop_token.hpp>

#include <chrono>

namespace score
{
namespace time
{

///
/// \brief Pure-virtual pimpl interface for the vehicle time domain backend.
///
/// INTERNAL — must not be included by user code.
/// Consumers: test mocks and backend implementations under details/.
///
class VehicleClockBackend
{
  public:
    virtual ~VehicleClockBackend() noexcept = default;

    /// \brief Returns the current vehicle time snapshot (time-point + quality status).
    virtual ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus> Now() const noexcept = 0;

    /// \brief Initialises the backend resource.
    ///
    /// Idempotent: a second call on an already-initialised backend returns \c true immediately.
    ///
    /// \return \c true on success; \c false on failure.
    virtual bool Init() noexcept = 0;

    /// \brief Returns true if the vehicle time backend resource is available.
    virtual bool IsAvailable() const noexcept = 0;

    /// \brief Blocks until the vehicle time resource is available or the deadline / stop-token fires.
    ///
    /// \param token  Stop token that can interrupt the wait.
    /// \param until  Steady-clock deadline after which the wait is abandoned.
    ///
    /// \return true if the resource became available, false if the wait was aborted.
    virtual bool WaitUntilAvailable(const score::cpp::stop_token& token,
                                    std::chrono::steady_clock::time_point until) const noexcept = 0;

    /// \brief Installs the callback invoked when new time-sync data arrives.
    virtual void SetTimeSlaveSyncDataReceivedCallback(
        VehicleTime::TimeSlaveSyncDataReceivedCallback&& callback) noexcept = 0;

    /// \brief Removes the time-sync data callback.
    virtual void UnsetTimeSlaveSyncDataReceivedCallback() noexcept = 0;

    /// \brief Installs the callback invoked after a finished pDelay measurement.
    virtual void SetPDelayMeasurementFinishedCallback(
        VehicleTime::PDelayMeasurementFinishedCallback&& callback) noexcept = 0;

    /// \brief Removes the pDelay measurement callback.
    virtual void UnsetPDelayMeasurementFinishedCallback() noexcept = 0;

    /// \brief Installs the callback invoked when VehicleTimeStatus flags change.
    ///
    /// The callback fires:
    ///  - unconditionally on the first \c Now() call after registration; and
    ///  - on every subsequent \c Now() call where the status flags differ from
    ///    the last fired value (rate deviation is ignored for comparison).
    ///
    /// The callback is invoked on the thread that calls \c Now() — typically
    /// a background polling thread.  The implementation must be thread-safe.
    virtual void SetStatusChangedCallback(
        VehicleTime::StatusChangedCallback&& callback) noexcept = 0;

    /// \brief Removes the status-changed callback.
    virtual void UnsetStatusChangedCallback() noexcept = 0;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_VEHICLE_CLOCK_BACKEND_H
