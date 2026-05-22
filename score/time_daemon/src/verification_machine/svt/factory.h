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
#ifndef SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_FACTORY_H
#define SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_FACTORY_H

#include "score/time_daemon/src/verification_machine/svt/svt_verification_machine.h"

namespace score
{
namespace td
{

/**
 * @brief Factory function to create a configured SvtVerificationMachine
 *
 * Creates and configures a SvtVerificationMachine with all necessary validators.
 *
 * @return A fully configured SvtVerificationMachine instance
 */
std::shared_ptr<SvtVerificationMachine> CreateSvtVerificationMachine(const std::string& name);

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_FACTORY_H
