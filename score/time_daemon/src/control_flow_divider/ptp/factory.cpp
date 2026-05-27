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
#include "score/time_daemon/src/common/logging_contexts.h"

#include "score/mw/log/logging.h"

namespace score
{
namespace td
{

std::shared_ptr<PtpControlFlowDivider> CreatePtpControlFlowDivider(const std::string& name,
                                                                   std::chrono::milliseconds timeout)
{
    return std::make_shared<PtpControlFlowDivider>(name, timeout);
}

}  // namespace td
}  // namespace score
