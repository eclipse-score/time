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
#ifndef SCORE_TIME_PTP_SRC_PDELAY_MEASUREMENT_DATA_H
#define SCORE_TIME_PTP_SRC_PDELAY_MEASUREMENT_DATA_H

#include "score/time/ptp/src/local_ptp_device_timer.h"
#include "score/time/ptp/src/master_ptp_device_timer.h"
#include "score/time/ptp/src/port_identity.h"

#include <chrono>
#include <cstdint>
#include <ostream>

namespace score
{
namespace time
{

///
/// @brief Data delivered after a pDelay measurement performed by a Time Slave.
///
/// @tparam Timebase  The clock domain tag (e.g. @c VehicleTime).
///
template <typename Timebase>
struct PDelayMeasurementData
{
    /// @brief Time Slave's local time at the point where it transmitted the pDelay Request Frame.
    LocalPTPDeviceTimerValue request_origin_timestamp{};

    /// @brief Time Master's timestamp at which it received the pDelay Request Frame.
    MasterPTPDeviceTimerValue request_receipt_timestamp{};

    /// @brief Time Master's timestamp at which it transmitted the pDelay Response Frame.
    MasterPTPDeviceTimerValue response_origin_timestamp{};

    /// @brief Time Slave's local time at the point where it received the pDelay Response Frame.
    LocalPTPDeviceTimerValue response_receipt_timestamp{};

    /// @brief Time Slave's global timestamp at which the pDelay measurement was initiated.
    typename Timebase::Timepoint reference_global_timestamp{};

    /// @brief Time Slave's local time at the point where it captured @c reference_global_timestamp.
    LocalPTPDeviceTimerValue reference_local_timestamp{};

    /// @brief Sequence number of the pDelay Request Frame.
    std::uint16_t sequence_id{};

    /// @brief Measured pDelay value.
    std::chrono::nanoseconds pdelay{};

    /// @brief Identity of the port from where the pDelay measurement was initiated.
    PortIdentity request_port_identity{};

    /// @brief Identity of the port that responded to the pDelay measurement.
    PortIdentity response_port_identity{};

    /// @brief Prints the data to any output stream.
    template <typename OutputStream>
    auto& PrintTo(OutputStream& output_stream) const
    {
        /* False positives: << incorrectly identified as bitwise operators, no address of a local variable is
         * returned. */
        return output_stream << "[" << request_origin_timestamp.time_since_epoch().count() << ", "
                             << request_receipt_timestamp.time_since_epoch().count() << ", "
                             << response_origin_timestamp.time_since_epoch().count() << ", "
                             << response_receipt_timestamp.time_since_epoch().count() << ", "
                             << reference_global_timestamp.time_since_epoch().count() << ", "
                             << reference_local_timestamp.time_since_epoch().count() << ", " << sequence_id << ", "
                             << pdelay.count() << ", " << request_port_identity << ", " << response_port_identity
                             << "]";
    }
};

/// @brief Compares two @c PDelayMeasurementData values field by field.
template <typename Timebase>
bool operator==(const PDelayMeasurementData<Timebase>& first, const PDelayMeasurementData<Timebase>& second) noexcept
{
    const bool same_request_origin_timestamp = (first.request_origin_timestamp == second.request_origin_timestamp);
    const bool same_request_receipt_timestamp = (first.request_receipt_timestamp == second.request_receipt_timestamp);
    const bool same_response_origin_timestamp = (first.response_origin_timestamp == second.response_origin_timestamp);
    const bool same_response_receipt_timestamp =
        (first.response_receipt_timestamp == second.response_receipt_timestamp);
    const bool same_reference_global_timestamp =
        (first.reference_global_timestamp == second.reference_global_timestamp);
    const bool same_reference_local_timestamp = (first.reference_local_timestamp == second.reference_local_timestamp);
    const bool same_sequence_id = (first.sequence_id == second.sequence_id);
    const bool same_pdelay = (first.pdelay == second.pdelay);
    const bool same_request_port_identity = (first.request_port_identity == second.request_port_identity);
    const bool same_response_port_identity = (first.response_port_identity == second.response_port_identity);
    return (same_request_origin_timestamp && same_request_receipt_timestamp && same_response_origin_timestamp &&
            same_response_receipt_timestamp && same_reference_global_timestamp && same_reference_local_timestamp &&
            same_sequence_id && same_pdelay && same_request_port_identity && same_response_port_identity);
}

/// @brief Inequality is the inverse of @c operator==.
template <typename Timebase>
bool operator!=(const PDelayMeasurementData<Timebase>& first, const PDelayMeasurementData<Timebase>& second) noexcept
{
    return !(first == second);
}

/// @brief Stream output operator for @c PDelayMeasurementData.
template <typename OutputStream, typename Timebase>
auto& operator<<(OutputStream& output_stream, const PDelayMeasurementData<Timebase>& pdelay_data)
{
    return pdelay_data.PrintTo(output_stream);
}

/// @brief GTest PrintTo overload for @c PDelayMeasurementData.
template <typename Timebase>
/* The dereference output_stream is passed as a non-const reference. Therefore, output_stream should be a pointer to a
 * non-const. */
void PrintTo(const PDelayMeasurementData<Timebase>& pdelay_data, std::ostream* const output_stream)
{
    pdelay_data.PrintTo(*output_stream);
}

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_PTP_SRC_PDELAY_MEASUREMENT_DATA_H
