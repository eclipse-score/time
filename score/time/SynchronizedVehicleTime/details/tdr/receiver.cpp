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
#include "score/time/SynchronizedVehicleTime/details/tdr/receiver.h"
#include "score/time/SynchronizedVehicleTime/details/logging_contexts.h"

#include "score/time/HighPrecisionLocalSteadyClock/details/factory_impl.h"

#include "score/time_daemon/src/ipc/svt/receiver/factory.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

#include "score/mw/log/logging.h"

#include <thread>

namespace score
{
namespace time
{
namespace details
{
namespace tdr
{

namespace
{
SynchronizedVehicleTime::TimepointStatus ConvertTimebaseStatus(
    const score::td::svt::TimeBaseStatus ptp_status)
{
    SynchronizedVehicleTime::TimepointStatus timepoint_status{};
    if (ptp_status.is_synchronized)
    {
        timepoint_status.AddFlag(SynchronizedVehicleTime::StatusFlag::kSynchronized);
    }

    if (ptp_status.is_timeout)
    {
        timepoint_status.AddFlag(SynchronizedVehicleTime::StatusFlag::kTimeOut);
    }

    if (ptp_status.is_time_jump_future)
    {
        timepoint_status.AddFlag(SynchronizedVehicleTime::StatusFlag::kTimeLeapFuture);
    }

    if (ptp_status.is_time_jump_past)
    {
        timepoint_status.AddFlag(SynchronizedVehicleTime::StatusFlag::kTimeLeapPast);
    }

    if (!ptp_status.is_correct)
    {
        timepoint_status.AddFlag(SynchronizedVehicleTime::StatusFlag::kUnknown);
    }

    return timepoint_status;
}
}  // namespace

using namespace std::chrono_literals;

Receiver::Receiver(std::unique_ptr<HighPrecisionLocalSteadyClock> high_precision_clock) noexcept
    : is_ready_{false}, local_clock_{std::move(high_precision_clock)}, svt_receiver_{score::td::CreateSvtReceiver()}
{
}

bool Receiver::WaitUntilAvailable(const score::cpp::stop_token& token,
                                  const std::chrono::time_point<std::chrono::steady_clock> until) const
{
    bool should_run = false;
    do
    {
        if (IsAvailable())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        should_run = (not token.stop_requested()) && (std::chrono::steady_clock::now() <= until);
    } while (should_run);

    score::mw::log::LogError(kSvtMainLogContext) << "The ipc to TimeDaemon could not be initialized!";
    return false;
}

bool Receiver::IsAvailable() const
{
    if (not(is_ready_))
    {
        is_ready_ = svt_receiver_->Init();
    }
    return is_ready_;
}

void Receiver::SetTimeSlaveSyncDataReceivedCallback(TimeSlaveSyncDataReceivedCallback&& /* callback */) noexcept {}

void Receiver::UnsetTimeSlaveSyncDataReceivedCallback() noexcept {}

void Receiver::SetPDelayMeasurementFinishedCallback(PDelayMeasurementFinishedCallback&& /* callback */) noexcept {}

void Receiver::UnsetPDelayMeasurementFinishedCallback() noexcept {}

SynchronizedVehicleTime::TimeStatus Receiver::Now() noexcept
{
    const SynchronizedVehicleTime::TimeStatus unknown_status{{}, SynchronizedVehicleTime::StatusFlag::kUnknown};

    if (is_ready_)
    {
        auto rx_data = svt_receiver_->Receive();
        if (rx_data.has_value())
        {
            const auto now_time = local_clock_->Now().time_since_epoch();
            const auto time_when_ptp_stamp_was_created = std::chrono::nanoseconds(rx_data.value().local_time);
            const auto ptp_stamp = std::chrono::nanoseconds(rx_data.value().ptp_assumed_time);
            // Check if current local clock value is grater than when PTP time stamp was created
            if (now_time >= time_when_ptp_stamp_was_created)
            {
                // Then adjust the ptp clock value the increase of local clock value
                auto adjusted_time = ptp_stamp + (now_time - time_when_ptp_stamp_was_created);

                // Convert it to known SVT timepoint type
                const SynchronizedVehicleTime::Timepoint converted_time{
                    std::chrono::duration_cast<SynchronizedVehicleTime::Timepoint::duration>(adjusted_time)};

                return {converted_time, ConvertTimebaseStatus(rx_data.value().status)};
            }
            else
            {
                score::mw::log::LogError(kSvtMainLogContext)
                    << "The received local time is higher than current time! Returning Unknown status";
            }
        }
    }

    return unknown_status;
}

double Receiver::GetRateDeviation() noexcept
{
    if (is_ready_)
    {
        auto rx_data = svt_receiver_->Receive();
        if (rx_data.has_value())
        {
            return rx_data.value().rate_deviation;
        }
    }
    return 0.0;
}

}  // namespace tdr
}  // namespace details
}  // namespace time
}  // namespace score
