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
#ifndef SCORE_TIME_DAEMON_SRC_CONTROL_FLOW_DIVIDER_PTP_FACTORY_H
#define SCORE_TIME_DAEMON_SRC_CONTROL_FLOW_DIVIDER_PTP_FACTORY_H

#include "score/time_daemon/src/control_flow_divider/ptp/ptp_control_flow_divider.h"

#include <chrono>
#include <memory>
#include <string>

namespace score
{
namespace td
{

/**
 * @brief Factory function to create a configured PtpControlFlowDivider.
 *
 * Creates and configures a PtpControlFlowDivider instance with the specified
 * parameters. The timeout determines how long the divider waits for new data
 * before publishing empty data to maintain consistent publishing rates.
 *
 * @param name The name of the control flow divider instance
 * @param timeout Maximum time to wait for new data before publishing empty data
 * @return A fully configured PtpControlFlowDivider instance
 */
std::shared_ptr<PtpControlFlowDivider> CreatePtpControlFlowDivider(const std::string& name,
                                                                   std::chrono::milliseconds timeout);

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_CONTROL_FLOW_DIVIDER_PTP_FACTORY_H
