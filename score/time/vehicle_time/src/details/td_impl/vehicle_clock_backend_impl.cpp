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
#include "score/time/vehicle_time/src/details/logging_contexts.h"

#include "score/time_daemon/src/ipc/svt/receiver/factory.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

#include "score/mw/log/logging.h"

#include <chrono>
#include <thread>

namespace score
{
namespace time
{
namespace detail
{

VehicleClockBackendImpl::VehicleClockBackendImpl(
    std::shared_ptr<score::td::SvtReceiver> receiver,
    HirsClock local_clock) noexcept
    : is_ready_{false}
    , init_mutex_{}
    , svt_receiver_{std::move(receiver)}
    , local_clock_{std::move(local_clock)}
{
}

ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>
VehicleClockBackendImpl::Now() const noexcept
{
    const ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus> kUnknownSnapshot{
        VehicleTime::Timepoint{},
        VehicleTimeStatus{
            ClockStatus<VehicleTime::StatusFlag>{{VehicleTime::StatusFlag::kUnknown}}, 0.0}};

    if (!is_ready_)
    {
        return kUnknownSnapshot;
    }

    const auto rx_data = svt_receiver_->Receive();
    if (!rx_data.has_value())
    {
        return kUnknownSnapshot;
    }

    const auto now_local        = local_clock_.Now().TimePoint().time_since_epoch();
    const auto local_at_capture = std::chrono::nanoseconds{rx_data.value().local_time};
    const auto ptp_at_capture   = std::chrono::nanoseconds{rx_data.value().ptp_assumed_time};

    if (now_local < local_at_capture)
    {
        score::mw::log::LogError(kVehicleTimeLogContext)
            << "Local clock is behind PTP capture reference — returning unknown status.";
        return kUnknownSnapshot;
    }

    const VehicleTime::Timepoint adjusted_tp{ptp_at_capture + (now_local - local_at_capture)};

    const VehicleTimeStatus vehicle_status{
        ConvertPtpStatus(rx_data.value().status), rx_data.value().rate_deviation};
    return ClockSnapshot<VehicleTime::Timepoint, VehicleTimeStatus>{adjusted_tp, vehicle_status};
}

bool VehicleClockBackendImpl::Init() noexcept
{
    if (is_ready_.load(std::memory_order_acquire))
    {
        return true;
    }

    const std::lock_guard<std::mutex> init_guard{init_mutex_};

    // Lets check if another thread completed init while we waited for the lock
    if (is_ready_.load(std::memory_order_relaxed))
    {
        return true;
    }

    const bool ok = svt_receiver_->Init();
    if (!ok)
    {
        score::mw::log::LogError(kVehicleTimeLogContext)
            << "VehicleClockBackendImpl: failed to open TimeDaemon shared memory segment.";
    }

    is_ready_.store(ok, std::memory_order_release);

    return ok;
}

bool VehicleClockBackendImpl::IsAvailable() const noexcept
{
    return is_ready_.load(std::memory_order_acquire);
}

bool VehicleClockBackendImpl::WaitUntilAvailable(
    const score::cpp::stop_token& token,
    std::chrono::steady_clock::time_point until) const noexcept
{
    bool should_poll = false;
    do
    {
        if (IsAvailable())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        should_poll = (!token.stop_requested()) && (std::chrono::steady_clock::now() <= until);
    } while (should_poll);

    score::mw::log::LogError(kVehicleTimeLogContext)
        << "Vehicle time IPC to TimeDaemon not ready within deadline.";
    return false;
}

void VehicleClockBackendImpl::SetTimeSlaveSyncDataReceivedCallback(
    VehicleTime::TimeSlaveSyncDataReceivedCallback&& /*callback*/) noexcept
{
    // Not yet supported by the TimeDaemon IPC subscription layer.
}

void VehicleClockBackendImpl::UnsetTimeSlaveSyncDataReceivedCallback() noexcept
{
    // Not yet supported.
}

void VehicleClockBackendImpl::SetPDelayMeasurementFinishedCallback(
    VehicleTime::PDelayMeasurementFinishedCallback&& /*callback*/) noexcept
{
    // Not yet supported.
}

void VehicleClockBackendImpl::UnsetPDelayMeasurementFinishedCallback() noexcept
{
    // Not yet supported.
}

ClockStatus<VehicleTime::StatusFlag> VehicleClockBackendImpl::ConvertPtpStatus(
    const score::td::svt::TimeBaseStatus& ptp_status) noexcept
{
    using Flag = VehicleTime::StatusFlag;
    ClockStatus<Flag> status;
    if (ptp_status.is_synchronized)
    {
        status.AddFlag(Flag::kSynchronized);
    }
    if (ptp_status.is_timeout)
    {
        status.AddFlag(Flag::kTimeOut);
    }
    if (ptp_status.is_time_jump_future)
    {
        status.AddFlag(Flag::kTimeLeapFuture);
    }
    if (ptp_status.is_time_jump_past)
    {
        status.AddFlag(Flag::kTimeLeapPast);
    }
    if (!ptp_status.is_correct)
    {
        status.AddFlag(Flag::kUnknown);
    }
    return status;
}

}  // namespace detail

template <>
std::shared_ptr<VehicleClockBackend> detail::CreateBackend<VehicleTime>()
{
    return std::make_shared<detail::VehicleClockBackendImpl>(score::td::CreateSvtReceiver(),
                                                      HirsClock::GetInstance());
}

}  // namespace time
}  // namespace score
