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
#ifndef SCORE_TIME_DAEMON_SRC_PTP_MACHINE_SHM_FACTORY_H
#define SCORE_TIME_DAEMON_SRC_PTP_MACHINE_SHM_FACTORY_H

#include "score/time_daemon/src/ptp_machine/shm/gptp_shm_machine.h"
#include "score/ts_client/src/gptp_ipc_channel.h"

#include <memory>
#include <string>

namespace score::td
{

/**
 * @brief Factory function to create a configured GPTPShmMachine.
 *
 * Creates a GPTPShmMachine backed by the shared-memory gPTP engine.
 * The engine reads PtpTimeInfo snapshots published by TimeSlave via
 * the IPC channel named @p ipc_name.
 *
 * @param name      Logical name for the machine instance.
 * @param ipc_name  IPC channel name (default: kGptpIpcName).
 * @return          A fully configured GPTPShmMachine instance.
 */
// kGptpIpcName is a char-array constant used as a default arg for a const std::string&; the
// decay is just the ordinary literal-to-temporary-std::string construction, not raw pointer use.
auto CreateGPTPShmMachine(const std::string& name,
                          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                          const std::string& ipc_name = score::ts::details::kGptpIpcName)
    -> std::shared_ptr<GPTPShmMachine>;

}  // namespace score::td

#endif  // SCORE_TIME_DAEMON_SRC_PTP_MACHINE_SHM_FACTORY_H
