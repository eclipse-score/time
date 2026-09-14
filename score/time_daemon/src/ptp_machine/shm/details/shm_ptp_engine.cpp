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

#include "score/mw/log/logging.h"
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/ts_client/src/gptp_ipc_data.h"
#include <string>
#include <utility>

namespace score::td::details
{

ShmPTPEngine::ShmPTPEngine(std::string ipc_name) noexcept : ipc_name_{std::move(ipc_name)} {}

auto ShmPTPEngine::Initialize() -> bool
{
    if (initialized_)
    {
        return true;
    }

    initialized_ = receiver_.Open(ipc_name_);
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

auto ShmPTPEngine::Deinitialize() -> bool
{
    if (initialized_)
    {
        receiver_.Close();
        initialized_ = false;
    }
    return true;
}

auto ShmPTPEngine::ReadPTPSnapshot(PtpTimeInfo& info) -> bool
{
    if (!initialized_)
    {
        return false;
    }

    auto result = receiver_.Receive();
    if (!result.has_value())
    {
        return false;
    }

    const score::ts::GptpIpcData& ipc_data = result.value();
    info.ptp_assumed_time = ipc_data.ptp_assumed_time;
    info.local_time = PtpTimeInfo::ReferenceClock::time_point{ipc_data.local_time};
    info.rate_deviation = ipc_data.rate_deviation;
    info.status.is_synchronized = ipc_data.status.is_synchronized;
    info.status.is_timeout = ipc_data.status.is_timeout;
    info.status.is_time_jump_future = ipc_data.status.is_time_jump_future;
    info.status.is_time_jump_past = ipc_data.status.is_time_jump_past;
    info.status.is_correct = ipc_data.status.is_correct;
    info.sync_fup_data.precise_origin_timestamp = ipc_data.sync_fup_data.precise_origin_timestamp;
    info.sync_fup_data.reference_global_timestamp = ipc_data.sync_fup_data.reference_global_timestamp;
    info.sync_fup_data.reference_local_timestamp = ipc_data.sync_fup_data.reference_local_timestamp;
    info.sync_fup_data.sync_ingress_timestamp = ipc_data.sync_fup_data.sync_ingress_timestamp;
    info.sync_fup_data.correction_field = ipc_data.sync_fup_data.correction_field;
    info.sync_fup_data.sequence_id = ipc_data.sync_fup_data.sequence_id;
    info.sync_fup_data.pdelay = ipc_data.sync_fup_data.pdelay;
    info.sync_fup_data.port_number = ipc_data.sync_fup_data.port_number;
    info.sync_fup_data.clock_identity = ipc_data.sync_fup_data.clock_identity;
    info.pdelay_data.request_origin_timestamp = ipc_data.pdelay_data.request_origin_timestamp;
    info.pdelay_data.request_receipt_timestamp = ipc_data.pdelay_data.request_receipt_timestamp;
    info.pdelay_data.response_origin_timestamp = ipc_data.pdelay_data.response_origin_timestamp;
    info.pdelay_data.response_receipt_timestamp = ipc_data.pdelay_data.response_receipt_timestamp;
    info.pdelay_data.reference_global_timestamp = ipc_data.pdelay_data.reference_global_timestamp;
    info.pdelay_data.reference_local_timestamp = ipc_data.pdelay_data.reference_local_timestamp;
    info.pdelay_data.sequence_id = ipc_data.pdelay_data.sequence_id;
    info.pdelay_data.pdelay = ipc_data.pdelay_data.pdelay;
    info.pdelay_data.req_port_number = ipc_data.pdelay_data.req_port_number;
    info.pdelay_data.req_clock_identity = ipc_data.pdelay_data.req_clock_identity;
    info.pdelay_data.resp_port_number = ipc_data.pdelay_data.resp_port_number;
    info.pdelay_data.resp_clock_identity = ipc_data.pdelay_data.resp_clock_identity;
    return true;
}

}  // namespace score::td::details
