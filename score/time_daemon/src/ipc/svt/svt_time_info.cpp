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
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

#include <score/assert.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

namespace score
{
namespace td
{
namespace svt
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

bool operator==(const TimeBaseStatus& first, const TimeBaseStatus& second) noexcept
{
    const bool same_sync = (first.is_synchronized == second.is_synchronized);
    const bool same_timeout = (first.is_timeout == second.is_timeout);
    const bool same_jump_future = (first.is_time_jump_future == second.is_time_jump_future);
    const bool same_jump_past = (first.is_time_jump_past == second.is_time_jump_past);
    const bool same_correct = (first.is_correct == second.is_correct);

    return (same_sync && same_timeout && same_jump_future && same_jump_past && same_correct);
}

bool operator==(const SyncFupSnapshot& first, const SyncFupSnapshot& second) noexcept
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

bool operator!=(const SyncFupSnapshot& first, const SyncFupSnapshot& second) noexcept
{
    return !(first == second);
}

bool operator==(const PDelayDataSnapshot& first, const PDelayDataSnapshot& second) noexcept
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

bool operator!=(const PDelayDataSnapshot& first, const PDelayDataSnapshot& second) noexcept
{
    return !(first == second);
}

bool operator==(const TimeBaseSnapshot& first, const TimeBaseSnapshot& second) noexcept
{
    const bool same_local = (first.local_time == second.local_time);
    const bool same_ptp = (first.ptp_assumed_time == second.ptp_assumed_time);
    const bool same_status = (first.status == second.status);
    const bool same_sync = (first.sync_fup_data == second.sync_fup_data);
    const bool same_pdelay = (first.pdelay_data == second.pdelay_data);
    const bool same_rate_deviation = NearlyEqual(first.rate_deviation, second.rate_deviation);

    return (same_local && same_ptp && same_status && same_sync && same_pdelay && same_rate_deviation);
}

bool operator!=(const TimeBaseSnapshot& first, const TimeBaseSnapshot& second) noexcept
{
    return !(first == second);
}

void TimeBaseSnapshot::CreateFrom(const PtpTimeInfo& info)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        info.ptp_assumed_time.count() >= 0,
        "score::td: CreateFrom(): ptp_assumed_time is negative");
    ptp_assumed_time = static_cast<uint64_t>(info.ptp_assumed_time.count());
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        info.local_time.time_since_epoch().count() >= 0,
        "score::td: CreateFrom(): local_time is negative");
    local_time = static_cast<uint64_t>(info.local_time.time_since_epoch().count());
    rate_deviation = info.rate_deviation;
    status.is_correct = info.status.is_correct;
    status.is_synchronized = info.status.is_synchronized;
    status.is_timeout = info.status.is_timeout;
    status.is_time_jump_future = info.status.is_time_jump_future;
    status.is_time_jump_past = info.status.is_time_jump_past;

    sync_fup_data.clock_identity = info.sync_fup_data.clock_identity;
    sync_fup_data.correction_field = info.sync_fup_data.correction_field;
    sync_fup_data.port_number = info.sync_fup_data.port_number;
    sync_fup_data.precise_origin_timestamp = info.sync_fup_data.precise_origin_timestamp;
    sync_fup_data.reference_global_timestamp = info.sync_fup_data.reference_global_timestamp;
    sync_fup_data.reference_local_timestamp = info.sync_fup_data.reference_local_timestamp;
    sync_fup_data.sequence_id = info.sync_fup_data.sequence_id;
    sync_fup_data.sync_ingress_timestamp = info.sync_fup_data.sync_ingress_timestamp;
    sync_fup_data.pdelay = info.sync_fup_data.pdelay;

    pdelay_data.req_clock_identity = info.pdelay_data.req_clock_identity;
    pdelay_data.req_port_number = info.pdelay_data.req_port_number;
    pdelay_data.request_origin_timestamp = info.pdelay_data.request_origin_timestamp;
    pdelay_data.request_receipt_timestamp = info.pdelay_data.request_receipt_timestamp;
    pdelay_data.response_origin_timestamp = info.pdelay_data.response_origin_timestamp;
    pdelay_data.response_receipt_timestamp = info.pdelay_data.response_receipt_timestamp;
    pdelay_data.reference_global_timestamp = info.pdelay_data.reference_global_timestamp;
    pdelay_data.reference_local_timestamp = info.pdelay_data.reference_local_timestamp;
    pdelay_data.sequence_id = info.pdelay_data.sequence_id;
    pdelay_data.pdelay = info.pdelay_data.pdelay;
    pdelay_data.resp_clock_identity = info.pdelay_data.resp_clock_identity;
    pdelay_data.resp_port_number = info.pdelay_data.resp_port_number;
}

bool operator==(const TimeBaseSnapshot& ipcdata, const PtpTimeInfo& data) noexcept
{
    const bool same_local = (ipcdata.local_time == static_cast<uint64_t>(data.local_time.time_since_epoch().count()));

    const bool same_ptp = (ipcdata.ptp_assumed_time == static_cast<uint64_t>(data.ptp_assumed_time.count()));

    const bool same_status = (ipcdata.status.is_correct == data.status.is_correct &&
                              ipcdata.status.is_synchronized == data.status.is_synchronized &&
                              ipcdata.status.is_timeout == data.status.is_timeout &&
                              ipcdata.status.is_time_jump_future == data.status.is_time_jump_future &&
                              ipcdata.status.is_time_jump_past == data.status.is_time_jump_past);

    const bool same_sync =
        (ipcdata.sync_fup_data.clock_identity == data.sync_fup_data.clock_identity &&
         ipcdata.sync_fup_data.correction_field == data.sync_fup_data.correction_field &&
         ipcdata.sync_fup_data.port_number == data.sync_fup_data.port_number &&
         ipcdata.sync_fup_data.precise_origin_timestamp == data.sync_fup_data.precise_origin_timestamp &&
         ipcdata.sync_fup_data.reference_global_timestamp == data.sync_fup_data.reference_global_timestamp &&
         ipcdata.sync_fup_data.reference_local_timestamp == data.sync_fup_data.reference_local_timestamp &&
         ipcdata.sync_fup_data.sequence_id == data.sync_fup_data.sequence_id &&
         ipcdata.sync_fup_data.sync_ingress_timestamp == data.sync_fup_data.sync_ingress_timestamp &&
         ipcdata.sync_fup_data.pdelay == data.sync_fup_data.pdelay);

    const bool same_pdelay =
        (ipcdata.pdelay_data.req_clock_identity == data.pdelay_data.req_clock_identity &&
         ipcdata.pdelay_data.req_port_number == data.pdelay_data.req_port_number &&
         ipcdata.pdelay_data.request_origin_timestamp == data.pdelay_data.request_origin_timestamp &&
         ipcdata.pdelay_data.request_receipt_timestamp == data.pdelay_data.request_receipt_timestamp &&
         ipcdata.pdelay_data.response_origin_timestamp == data.pdelay_data.response_origin_timestamp &&
         ipcdata.pdelay_data.response_receipt_timestamp == data.pdelay_data.response_receipt_timestamp &&
         ipcdata.pdelay_data.reference_global_timestamp == data.pdelay_data.reference_global_timestamp &&
         ipcdata.pdelay_data.reference_local_timestamp == data.pdelay_data.reference_local_timestamp &&
         ipcdata.pdelay_data.sequence_id == data.pdelay_data.sequence_id &&
         ipcdata.pdelay_data.pdelay == data.pdelay_data.pdelay &&
         ipcdata.pdelay_data.resp_clock_identity == data.pdelay_data.resp_clock_identity &&
         ipcdata.pdelay_data.resp_port_number == data.pdelay_data.resp_port_number);

    const bool same_rate_deviation = NearlyEqual(ipcdata.rate_deviation, data.rate_deviation);

    return (same_local && same_ptp && same_status && same_sync && same_pdelay && same_rate_deviation);
}

bool operator!=(const TimeBaseSnapshot& ipcdata, const PtpTimeInfo& data) noexcept
{
    return !(ipcdata == data);
}

/// \brief  gtest compatibility:
void PrintTo(const TimeBaseStatus& status, std::ostream* os)
{
    std::ignore = PrintTo(status, *os);
}

void PrintTo(const SyncFupSnapshot& data, std::ostream* os)
{
    std::ignore = PrintTo(data, *os);
}

void PrintTo(const PDelayDataSnapshot& data, std::ostream* os)
{
    std::ignore = PrintTo(data, *os);
}

void PrintTo(const TimeBaseSnapshot& info, std::ostream* os)
{
    std::ignore = PrintTo(info, *os);
}

}  // namespace svt
}  // namespace td
}  // namespace score
