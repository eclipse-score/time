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
#include "score/time_daemon/src/verification_machine/svt/validators/synchronization_validator.h"
#include "score/mw/log/logging.h"
#include "score/time_daemon/src/common/logging_contexts.h"

namespace score
{
namespace td
{

SynchronizationValidator::SynchronizationValidator() : is_synchronized_{false} {}

void SynchronizationValidator::DoValidation(PtpTimeInfo& data)
{
    // If we are already synchronized, ensure is_synchronized is always set
    if (is_synchronized_)
    {
        data.status.is_synchronized = true;
        score::mw::log::LogDebug(kVerificationMachineContext)
            << "SynchronizationValidator: Set synchronized flag to true.";
        return;
    }

    // If we were never synchronized, then store the flag for further calls
    if (data.status.is_synchronized)
    {
        is_synchronized_ = true;
        score::mw::log::LogInfo(kVerificationMachineContext) << "SynchronizationValidator: Synchronization achieved.";
    }
}

}  // namespace td
}  // namespace score
