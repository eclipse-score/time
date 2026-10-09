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
#include "score/time_daemon/src/application/svt/svt_handler.h"
#include "score/time_daemon/src/control_flow_divider/ptp/factory.h"
#include "score/time_daemon/src/ipc/svt/publisher/factory.h"
#include "score/time_daemon/src/msg_broker/topic.h"
#include "score/time_daemon/src/ptp_machine/shm/factory.h"
#include "score/time_daemon/src/verification_machine/svt/factory.h"

#include <chrono>

namespace score
{
namespace td
{

SvtHandler::SvtHandler() noexcept : GenericTimebaseHandler<PtpTimeInfo>{"svt"} {}

void SvtHandler::CreateComponents() noexcept
{
    // Added first so that the control flow divider starts before the gPTP machine publishes data.
    auto ctrl_flow_divider =
        Add(CreatePtpControlFlowDivider("ptp_control_flow_divider", std::chrono::milliseconds{250}));
    auto gptp_machine = Add(CreateGPTPShmMachine("ptp_worker"));
    auto verification_machine = Add(CreateSvtVerificationMachine("time_verification_worker"));
    auto ipc_publisher = Add(CreateSvtPublisher("svt_ipc_publisher"));

    const auto input_ptp_data_topic = Topic("in_ptp_data");
    const auto raw_ptp_data_topic = Topic("raw_ptp_data");
    const auto validated_ptp_data_topic = Topic("validated_ptp_data");

    Subscribe(input_ptp_data_topic, ctrl_flow_divider);
    Subscribe(raw_ptp_data_topic, verification_machine);
    Subscribe(validated_ptp_data_topic, ipc_publisher);

    Publish(gptp_machine, input_ptp_data_topic);
    Publish(ctrl_flow_divider, raw_ptp_data_topic);
    Publish(verification_machine, validated_ptp_data_topic);
}

}  // namespace td
}  // namespace score
