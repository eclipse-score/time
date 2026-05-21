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
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

namespace score
{
namespace td
{

namespace
{

bool NearlyEqual(const double first, const double second) noexcept
{
    const double diff = std::fabs(first - second);
    return diff <= std::numeric_limits<double>::epsilon();
}

}  // namespace

/// \brief Comparing operators:

bool operator==(const PtpStatus& first, const PtpStatus& second) noexcept
{
    const bool same_sync = (first.is_synchronized == second.is_synchronized);
    const bool same_timeout = (first.is_timeout == second.is_timeout);
    const bool same_jump_future = (first.is_time_jump_future == second.is_time_jump_future);
    const bool same_jump_past = (first.is_time_jump_past == second.is_time_jump_past);
    const bool same_unknown = (first.is_correct == second.is_correct);

    return (same_sync && same_timeout && same_jump_future && same_jump_past && same_unknown);
}

bool operator==(const SyncFupData& first, const SyncFupData& second) noexcept
{
    const bool same_precise_origin_timestamp = (first.precise_origin_timestamp == second.precise_origin_timestamp);
    const bool same_reference_global_timestamp =
        (first.reference_global_timestamp == second.reference_global_timestamp);
    const bool same_reference_local_timestamp = (first.reference_local_timestamp == second.reference_local_timestamp);
    const bool same_sync_ingress_timestamp = (first.sync_ingress_timestamp == second.sync_ingress_timestamp);
    const bool same_correction_field = (first.correction_field == second.correction_field);
    const bool same_sequence_id = (first.sequence_id == second.sequence_id);
    const bool same_pdelay = (first.pdelay == second.pdelay);
    const bool same_port_number = (first.port_number == second.port_number);
    const bool same_clock_identity = (first.clock_identity == second.clock_identity);
    return (same_precise_origin_timestamp && same_reference_global_timestamp && same_reference_local_timestamp &&
            same_sync_ingress_timestamp && same_correction_field && same_sequence_id && same_pdelay &&
            same_port_number && same_clock_identity);
}

bool operator!=(const SyncFupData& first, const SyncFupData& second) noexcept
{
    return !(first == second);
}

bool operator==(const PDelayData& first, const PDelayData& second) noexcept
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
    const bool same_req_port_number = (first.req_port_number == second.req_port_number);
    const bool same_req_clock_identity = (first.req_clock_identity == second.req_clock_identity);
    const bool same_resp_port_number = (first.resp_port_number == second.resp_port_number);
    const bool same_resp_clock_identity = (first.resp_clock_identity == second.resp_clock_identity);
    return (same_request_origin_timestamp && same_request_receipt_timestamp && same_response_origin_timestamp &&
            same_response_receipt_timestamp && same_reference_global_timestamp && same_reference_local_timestamp &&
            same_sequence_id && same_pdelay && same_req_port_number && same_req_clock_identity &&
            same_resp_port_number && same_resp_clock_identity);
}

bool operator!=(const PDelayData& first, const PDelayData& second) noexcept
{
    return !(first == second);
}

bool operator==(const PtpTimeInfo& first, const PtpTimeInfo& second) noexcept
{
    const bool same_local = (first.local_time == second.local_time);
    const bool same_ptp = (first.ptp_assumed_time == second.ptp_assumed_time);
    const bool same_rate_deviation = NearlyEqual(first.rate_deviation, second.rate_deviation);
    const bool same_status = (first.status == second.status);
    const bool same_sync = (first.sync_fup_data == second.sync_fup_data);
    const bool same_pdelay = (first.pdelay_data == second.pdelay_data);

    return (same_local && same_ptp && same_rate_deviation && same_status && same_sync && same_pdelay);
}

bool operator!=(const PtpTimeInfo& first, const PtpTimeInfo& second) noexcept
{
    return !(first == second);
}

/// \brief  gtest compatibility:
void PrintTo(const PtpStatus& status, std::ostream* os)
{
    std::ignore = PrintTo(status, *os);
}

void PrintTo(const SyncFupData& data, std::ostream* os)
{
    std::ignore = PrintTo(data, *os);
}

void PrintTo(const PDelayData& data, std::ostream* os)
{
    std::ignore = PrintTo(data, *os);
}

void PrintTo(const PtpTimeInfo& info, std::ostream* os)
{
    std::ignore = PrintTo(info, *os);
}

}  // namespace td
}  // namespace score
