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
#ifndef SCORE_TIMEDAEMON_CODE_PTP_MACHINE_SHM_GPTP_SHM_MACHINE_H
#define SCORE_TIMEDAEMON_CODE_PTP_MACHINE_SHM_GPTP_SHM_MACHINE_H

#include "score/TimeDaemon/code/ptp_machine/core/ptp_machine.h"
#include "score/TimeDaemon/code/ptp_machine/shm/details/shm_ptp_engine.h"

namespace score
{
namespace td
{

/// @brief PTPMachine instantiated with the shared-memory gPTP engine.
///
/// Reads PtpTimeInfo snapshots written by TimeSlave via the IPC channel.
/// Construct via CreateGPTPShmMachine() (see factory.h) or directly:
///
/// @code
/// auto machine = std::make_shared<GPTPShmMachine>(
///     "shm", std::chrono::milliseconds{50}, "/gptp_ptp_info");
/// @endcode
using GPTPShmMachine = PTPMachine<details::ShmPTPEngine>;

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_PTP_MACHINE_SHM_GPTP_SHM_MACHINE_H
