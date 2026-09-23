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
#ifndef SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CONVERTERS_H
#define SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CONVERTERS_H

#include "score/time/vehicle_time/src/vehicle_clock.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

namespace score
{
namespace time
{
namespace detail
{

/// @brief Converts PTP status flags from the TimeDaemon IPC representation to
///        the @c ClockStatus<VehicleTime::StatusFlag> representation.
///
/// An IPC status that is not marked correct yields an empty status (no flags set).
ClockStatus<VehicleTime::StatusFlag> ConvertPtpStatus(const score::td::svt::TimeBaseStatus& ptp_status) noexcept;

/// @brief Converts the IPC sync/follow-up snapshot to the public @c TimeSlaveSyncData event type.
TimeSlaveSyncData<VehicleTime> ConvertSyncData(const score::td::svt::SyncFupSnapshot& sync_data) noexcept;

/// @brief Converts the IPC pDelay snapshot to the public @c PDelayMeasurementData event type.
PDelayMeasurementData<VehicleTime> ConvertPDelayData(const score::td::svt::PDelayDataSnapshot& pdelay_data) noexcept;

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CONVERTERS_H
