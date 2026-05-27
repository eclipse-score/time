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
#ifndef SCORE_TIME_SYNCHRONIZEDVEHICLETIME_DETAILS_TDR_RECEIVER_H
#define SCORE_TIME_SYNCHRONIZEDVEHICLETIME_DETAILS_TDR_RECEIVER_H

#include "score/time/SynchronizedVehicleTime/synchronized_vehicle_time.h"

#include "score/time_daemon/src/ipc/svt/receiver/svt_receiver.h"

#include "score/time/HighPrecisionLocalSteadyClock/details/factory_impl.h"
#include "score/time/HighPrecisionLocalSteadyClock/high_precision_local_steady_clock.h"

#include <atomic>
#include <chrono>

namespace score
{
namespace time
{
namespace details
{
namespace tdr
{

///
/// \brief Class to implement SynchronizedVehicleTime clock with using Time Daemon receiver implementation.
///

class Receiver final : public score::time::SynchronizedVehicleTime
{
  public:
    explicit Receiver(std::unique_ptr<HighPrecisionLocalSteadyClock> local_clock_ =
                          HighPrecisionLocalSteadyClock::FactoryImpl().CreateHighPrecisionLocalSteadyClock()) noexcept;
    Receiver& operator=(const Receiver&) & noexcept = delete;
    Receiver& operator=(Receiver&&) & noexcept = delete;
    Receiver(const Receiver&) noexcept = delete;
    Receiver(Receiver&&) noexcept = delete;
    ~Receiver() = default;

    /// \brief Method for waiting until the timebase is available
    ///
    /// \details overrides score::time::SynchronizedVehicleTime::WaitUntilAvailable()
    ///
    bool WaitUntilAvailable(const score::cpp::stop_token& token,
                            const std::chrono::time_point<std::chrono::steady_clock> until) const override;

    /// \brief Method to check if timebase is available
    ///
    /// \return true in case the receiver was initalized successfully, false otherwise or if mutex acquisition failed
    ///
    bool IsAvailable() const override;

    /// \brief Method that is intended to be called by users of score::time::SynchronizedVehicleTime for
    ///        subscribing to notification about updates of time synchronization data.
    ///
    /// \details overrides score::time::SynchronizedVehicleTime::SetTimeSlaveSyncDataReceivedCallback()
    ///
    void SetTimeSlaveSyncDataReceivedCallback(TimeSlaveSyncDataReceivedCallback&& /* callback */) noexcept override;

    /// \brief Method that is intended to be called by users of score::time::SynchronizedVehicleTime for
    ///        unsubscribing from notification about updates of time synchronization data.
    ///
    /// \details overrides score::time::SynchronizedVehicleTime::UnsetTimeSlaveSyncDataReceivedCallback()
    ///
    void UnsetTimeSlaveSyncDataReceivedCallback() noexcept override;

    /// \brief Method that is intended to be called by users of score::time::SynchronizedVehicleTime for
    ///        subscribing to notification about updates of PDelay measurement data.
    ///
    /// \details overrides score::time::SynchronizedVehicleTime::SetPDelayMeasurementFinishedCallback()
    ///
    void SetPDelayMeasurementFinishedCallback(PDelayMeasurementFinishedCallback&& /* callback */) noexcept override;

    /// \brief Method that is intended to be called by users of score::time::SynchronizedVehicleTime for
    ///        unsubscribing from notification about updates of PDelay measurement data.
    ///
    /// \details overrides score::time::SynchronizedVehicleTime::UnsetPDelayMeasurementFinishedCallback()
    ///
    void UnsetPDelayMeasurementFinishedCallback() noexcept override;

    /// \brief Method that is intended to be called by users of score::time::SynchronizedVehicleTime
    ///        for gathering the current clock value and timebase status of the underlying timebase
    ///
    /// \details overrides score::time::SynchronizedVehicleTime::Now()
    ///
    TimeStatus Now() noexcept override;

    /// \brief Method for obtaining the current deviation rate of the underlying timebase
    ///
    /// \details overrides score::time::SynchronizedVehicleTime::GetRateDeviation()
    double GetRateDeviation() noexcept override;

  private:
    mutable std::atomic_bool is_ready_;
    std::unique_ptr<HighPrecisionLocalSteadyClock> local_clock_;
    std::shared_ptr<score::td::SvtReceiver> svt_receiver_;
};

}  // namespace tdr
}  // namespace details
}  // namespace time
}  // namespace score

#endif  // #ifndef SCORE_TIME_SYNCHRONIZEDVEHICLETIME_DETAILS_TDR_RECEIVER_H
