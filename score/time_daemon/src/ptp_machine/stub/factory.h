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
#ifndef SCORE_TIME_DAEMON_SRC_PTP_MACHINE_STUB_FACTORY_H
#define SCORE_TIME_DAEMON_SRC_PTP_MACHINE_STUB_FACTORY_H

#include "score/time_daemon/src/ptp_machine/stub/gptp_stub_machine.h"

namespace score
{
namespace td
{

/**
 * @brief Factory function to create a configured GPTPStubMachine
 *
 * Creates and configures a GPTPStubMachine.
 *
 * @return A fully configured GPTPStubMachine instance
 */
std::shared_ptr<GPTPStubMachine> CreateGPTPStubMachine(const std::string& name);

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_PTP_MACHINE_STUB_FACTORY_H
