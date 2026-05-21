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
#ifndef SCORE_TIMEDAEMON_CODE_CONTROL_FLOW_DIVIDER_PTP_PTP_CONTROL_FLOW_DIVIDER_H
#define SCORE_TIMEDAEMON_CODE_CONTROL_FLOW_DIVIDER_PTP_PTP_CONTROL_FLOW_DIVIDER_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/control_flow_divider/core/control_flow_divider.h"

#include <chrono>

namespace score
{
namespace td
{

/**
 * @brief ControlFlowDivider specialized for PTP time information.
 *
 * This component separates the control flow between the Machines.
 */
constexpr size_t kBufferSize = 10U;
using PtpControlFlowDivider = ControlFlowDivider<PtpTimeInfo, kBufferSize>;

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_CONTROL_FLOW_DIVIDER_PTP_PTP_CONTROL_FLOW_DIVIDER_H
