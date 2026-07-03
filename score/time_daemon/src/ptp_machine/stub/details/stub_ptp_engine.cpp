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
#include "score/time_daemon/src/ptp_machine/stub/details/stub_ptp_engine.h"
#include "score/mw/log/logging.h"
#include "score/time_daemon/src/common/logging_contexts.h"

#include <array>
#include <numeric>

namespace score
{
namespace td
{
namespace details
{

namespace
{

std::uint16_t sequence_id_{0U};

}  // namespace

StubPTPEngine::StubPTPEngine(PtpTimeInfo::ReferenceClock local_clock) noexcept : local_clock_{std::move(local_clock)}
{
    score::mw::log::LogInfo(kGPtpMachineContext) << "StubPTPEngine created!";
}

bool StubPTPEngine::Initialize() const
{
    score::mw::log::LogInfo(kGPtpMachineContext) << "StubPTPEngine initialization succeeded!";

    return true;
}

bool StubPTPEngine::Deinitialize() const
{
    score::mw::log::LogInfo(kGPtpMachineContext) << "StubPTPEngine deinitialization succeeded!";
    return true;
}

bool StubPTPEngine::ReadPTPSnapshot(PtpTimeInfo& info)
{
    const bool time_status_ok = ReadTimeValueAndStatus(info);
    const bool pdelay_ok = ReadPDelayMeasurementData(info);
    const bool sync_ok = ReadSyncMeasurementData(info);

    return (time_status_ok && pdelay_ok && sync_ok);
}

bool StubPTPEngine::ReadTimeValueAndStatus(PtpTimeInfo& time_info) noexcept
{
    const auto snapshot = local_clock_.Now();
    time_info.local_time = snapshot.TimePoint();
    time_info.ptp_assumed_time = snapshot.TimeSinceEpoch();
    time_info.rate_deviation = 0.0;
    time_info.status = PtpStatus{true, false, false, false, true};

    ++sequence_id_;

    return true;
}

bool StubPTPEngine::ReadSyncMeasurementData(PtpTimeInfo& time_info) const noexcept
{
    // Stub: timestamps derived from local clock so they increase monotonically
    const auto now_ns = static_cast<std::uint64_t>(local_clock_.Now().TimeSinceEpoch().count());

    time_info.sync_fup_data.precise_origin_timestamp = now_ns;
    time_info.sync_fup_data.reference_global_timestamp = now_ns;
    time_info.sync_fup_data.reference_local_timestamp = now_ns;
    time_info.sync_fup_data.sync_ingress_timestamp = now_ns;
    time_info.sync_fup_data.correction_field = 0U;
    time_info.sync_fup_data.sequence_id = sequence_id_;
    time_info.sync_fup_data.pdelay = 1'000U;  // 1 µs simulated pdelay
    time_info.sync_fup_data.port_number = 1U;
    time_info.sync_fup_data.clock_identity = 0xAABBCCDDEEFF0011ULL;

    return true;
}

bool StubPTPEngine::ReadPDelayMeasurementData(PtpTimeInfo& time_info) const noexcept
{
    // Stub: simulate a round-trip with 1 µs one-way pdelay anchored to local clock
    const auto now_ns = static_cast<std::uint64_t>(local_clock_.Now().TimeSinceEpoch().count());
    constexpr std::uint64_t kOnewayDelayNs{1'000U};  // 1 µs simulated one-way pdelay

    time_info.pdelay_data.request_origin_timestamp = now_ns;
    time_info.pdelay_data.request_receipt_timestamp = now_ns + kOnewayDelayNs;
    time_info.pdelay_data.response_origin_timestamp = now_ns + kOnewayDelayNs;
    time_info.pdelay_data.response_receipt_timestamp = now_ns + 2U * kOnewayDelayNs;
    time_info.pdelay_data.reference_global_timestamp = now_ns;
    time_info.pdelay_data.reference_local_timestamp = now_ns;
    time_info.pdelay_data.sequence_id = sequence_id_;
    time_info.pdelay_data.pdelay = kOnewayDelayNs;
    time_info.pdelay_data.req_port_number = 1U;
    time_info.pdelay_data.req_clock_identity = 0xAABBCCDDEEFF0011ULL;
    time_info.pdelay_data.resp_port_number = 2U;
    time_info.pdelay_data.resp_clock_identity = 0x1122334455667788ULL;

    return true;
}

}  // namespace details
}  // namespace td
}  // namespace score
