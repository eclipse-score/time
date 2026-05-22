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
#ifndef SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_SVT_VERIFICATION_MACHINE_H
#define SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_SVT_VERIFICATION_MACHINE_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/verification_machine/core/verification_machine.h"
#include "score/time_daemon/src/verification_machine/svt/validators/synchronization_validator.h"
#include "score/time_daemon/src/verification_machine/svt/validators/time_jumps_validator.h"
#include "score/time_daemon/src/verification_machine/svt/validators/timeout_validator.h"

namespace score
{
namespace td
{

/**
 * @brief Machine component responsible for validating and qualifying time information.
 */
using SvtVerificationMachine = VerificationMachine<PtpTimeInfo>;

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_SVT_VERIFICATION_MACHINE_H
