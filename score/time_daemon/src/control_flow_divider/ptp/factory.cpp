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
#include "score/time_daemon/src/control_flow_divider/ptp/factory.h"

#include "score/time_daemon/src/control_flow_divider/ptp/ptp_control_flow_divider.h"
#include <chrono>
#include <memory>
#include <string>

namespace score::td
{

auto CreatePtpControlFlowDivider(const std::string& name, std::chrono::milliseconds timeout)
    -> std::shared_ptr<PtpControlFlowDivider>
{
    return std::make_shared<PtpControlFlowDivider>(name, timeout);
}

}  // namespace score::td
