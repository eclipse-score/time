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
#ifndef SCORE_TIMEDAEMON_CODE_COMMON_LOGGING_CONTEXTS_H
#define SCORE_TIMEDAEMON_CODE_COMMON_LOGGING_CONTEXTS_H

#include <string>

namespace score
{
namespace td
{

// Application context
constexpr auto kAppContext = "TDAP";
constexpr auto kTimeBaseHandlerSvt = "TSVT";
constexpr auto kMessageBrokerContext = "MSGB";

// PTP Machine contexts
constexpr auto kPtpMachineContext = "PTPM";
constexpr auto kGPtpMachineContext = "GPTP";

// IPC Handler context
constexpr auto kIpcHandlerContext = "IPCH";

// Verification Machine contexts
constexpr auto kVerificationMachineContext = "VERM";

// Control Flow Divider context
constexpr auto kControlFlowDividerContext = "CFDV";

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_COMMON_LOGGING_CONTEXTS_H
