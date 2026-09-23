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
#include "score/time/vehicle_time/src/details/td_impl/svt_converters.h"

#include <chrono>
#include <cstdint>

namespace score
{
namespace time
{
namespace detail
{

namespace
{

/// @brief Reinterprets an unsigned nanosecond count from the IPC layer as a signed chrono duration.
std::chrono::nanoseconds ToNanoseconds(const std::uint64_t nanoseconds) noexcept
{
    return std::chrono::nanoseconds{static_cast<std::chrono::nanoseconds::rep>(nanoseconds)};
}

PortIdentity ToPortIdentity(const std::uint64_t clock_identity, const std::uint32_t port_number) noexcept
{
    PortIdentity identity{};
    identity.clock_identity = clock_identity;
    identity.port_number = static_cast<std::uint16_t>(port_number);
    return identity;
}

}  // namespace

ClockStatus<VehicleTime::StatusFlag> ConvertPtpStatus(const score::td::svt::TimeBaseStatus& ptp_status) noexcept
{
    using Flag = VehicleTime::StatusFlag;
    if (!ptp_status.is_correct)
    {
        return ClockStatus<Flag>{};
    }
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
    return status;
}

TimeSlaveSyncData<VehicleTime> ConvertSyncData(const score::td::svt::SyncFupSnapshot& sync_data) noexcept
{
    TimeSlaveSyncData<VehicleTime> converted{};
    converted.precise_origin_timestamp = VehicleTime::Timepoint{ToNanoseconds(sync_data.precise_origin_timestamp)};
    converted.reference_global_timestamp = VehicleTime::Timepoint{ToNanoseconds(sync_data.reference_global_timestamp)};
    converted.reference_local_timestamp = LocalPTPDeviceTimerValue{ToNanoseconds(sync_data.reference_local_timestamp)};
    converted.sync_ingress_timestamp = LocalPTPDeviceTimerValue{ToNanoseconds(sync_data.sync_ingress_timestamp)};
    converted.correction_field = static_cast<std::int64_t>(sync_data.correction_field);
    converted.sequence_id = sync_data.sequence_id;
    converted.pdelay = ToNanoseconds(sync_data.pdelay);
    converted.source_port_identity = ToPortIdentity(sync_data.clock_identity, sync_data.port_number);
    return converted;
}

PDelayMeasurementData<VehicleTime> ConvertPDelayData(const score::td::svt::PDelayDataSnapshot& pdelay_data) noexcept
{
    PDelayMeasurementData<VehicleTime> converted{};
    converted.request_origin_timestamp = LocalPTPDeviceTimerValue{ToNanoseconds(pdelay_data.request_origin_timestamp)};
    converted.request_receipt_timestamp =
        MasterPTPDeviceTimerValue{ToNanoseconds(pdelay_data.request_receipt_timestamp)};
    converted.response_origin_timestamp =
        MasterPTPDeviceTimerValue{ToNanoseconds(pdelay_data.response_origin_timestamp)};
    converted.response_receipt_timestamp =
        LocalPTPDeviceTimerValue{ToNanoseconds(pdelay_data.response_receipt_timestamp)};
    converted.reference_global_timestamp =
        VehicleTime::Timepoint{ToNanoseconds(pdelay_data.reference_global_timestamp)};
    converted.reference_local_timestamp =
        LocalPTPDeviceTimerValue{ToNanoseconds(pdelay_data.reference_local_timestamp)};
    converted.sequence_id = pdelay_data.sequence_id;
    converted.pdelay = ToNanoseconds(pdelay_data.pdelay);
    converted.request_port_identity = ToPortIdentity(pdelay_data.req_clock_identity, pdelay_data.req_port_number);
    converted.response_port_identity = ToPortIdentity(pdelay_data.resp_clock_identity, pdelay_data.resp_port_number);
    return converted;
}

}  // namespace detail
}  // namespace time
}  // namespace score
