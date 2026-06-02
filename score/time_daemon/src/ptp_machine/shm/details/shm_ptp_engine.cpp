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
#include "score/time_daemon/src/ptp_machine/shm/details/shm_ptp_engine.h"

#include "score/time_daemon/src/common/logging_contexts.h"
#include "score/ts_client/src/gptp_ipc_data.h"
#include "score/mw/log/logging.h"

namespace score
{
namespace td
{
namespace details
{

ShmPTPEngine::ShmPTPEngine(std::string ipc_name) noexcept : ipc_name_{std::move(ipc_name)} {}

bool ShmPTPEngine::Initialize()
{
    if (initialized_)
        return true;

    initialized_ = receiver_.Init(ipc_name_);
    if (initialized_)
    {
        score::mw::log::LogInfo(kGPtpMachineContext) << "ShmPTPEngine: connected to IPC channel " << ipc_name_;
    }
    else
    {
        score::mw::log::LogError(kGPtpMachineContext) << "ShmPTPEngine: failed to open IPC channel " << ipc_name_;
    }
    return initialized_;
}

bool ShmPTPEngine::Deinitialize()
{
    if (initialized_)
    {
        receiver_.Close();
        initialized_ = false;
    }
    return true;
}

bool ShmPTPEngine::ReadPTPSnapshot(PtpTimeInfo& info)
{
    if (!initialized_)
        return false;

    auto result = receiver_.Receive();
    if (!result.has_value())
        return false;

    const score::ts::GptpIpcData& d = result.value();
    info.ptp_assumed_time = d.ptp_assumed_time;
    info.local_time = PtpTimeInfo::ReferenceClock::time_point{d.local_time};
    info.rate_deviation = d.rate_deviation;
    info.status.is_synchronized = d.status.is_synchronized;
    info.status.is_timeout = d.status.is_timeout;
    info.status.is_time_jump_future = d.status.is_time_jump_future;
    info.status.is_time_jump_past = d.status.is_time_jump_past;
    info.status.is_correct = d.status.is_correct;
    info.sync_fup_data.precise_origin_timestamp = d.sync_fup_data.precise_origin_timestamp;
    info.sync_fup_data.reference_global_timestamp = d.sync_fup_data.reference_global_timestamp;
    info.sync_fup_data.reference_local_timestamp = d.sync_fup_data.reference_local_timestamp;
    info.sync_fup_data.sync_ingress_timestamp = d.sync_fup_data.sync_ingress_timestamp;
    info.sync_fup_data.correction_field = d.sync_fup_data.correction_field;
    info.sync_fup_data.sequence_id = d.sync_fup_data.sequence_id;
    info.sync_fup_data.pdelay = d.sync_fup_data.pdelay;
    info.sync_fup_data.port_number = d.sync_fup_data.port_number;
    info.sync_fup_data.clock_identity = d.sync_fup_data.clock_identity;
    info.pdelay_data.request_origin_timestamp = d.pdelay_data.request_origin_timestamp;
    info.pdelay_data.request_receipt_timestamp = d.pdelay_data.request_receipt_timestamp;
    info.pdelay_data.response_origin_timestamp = d.pdelay_data.response_origin_timestamp;
    info.pdelay_data.response_receipt_timestamp = d.pdelay_data.response_receipt_timestamp;
    info.pdelay_data.reference_global_timestamp = d.pdelay_data.reference_global_timestamp;
    info.pdelay_data.reference_local_timestamp = d.pdelay_data.reference_local_timestamp;
    info.pdelay_data.sequence_id = d.pdelay_data.sequence_id;
    info.pdelay_data.pdelay = d.pdelay_data.pdelay;
    info.pdelay_data.req_port_number = d.pdelay_data.req_port_number;
    info.pdelay_data.req_clock_identity = d.pdelay_data.req_clock_identity;
    info.pdelay_data.resp_port_number = d.pdelay_data.resp_port_number;
    info.pdelay_data.resp_clock_identity = d.pdelay_data.resp_clock_identity;
    return true;
}

}  // namespace details
}  // namespace td
}  // namespace score
