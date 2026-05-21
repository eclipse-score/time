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
#ifndef SCORE_TIMEDAEMON_CODE_COMMON_MACHINES_REACTIVE_MACHINE_H
#define SCORE_TIMEDAEMON_CODE_COMMON_MACHINES_REACTIVE_MACHINE_H

#include "score/time_daemon/src/common/machines/base_machine.h"

namespace score
{
namespace td
{

/**
 * @brief Base class for machine components that react to system events.
 *
 * ReactiveMachine serves as the foundation for components that respond to
 * messages or events produced by other components rather than actively
 * generating data. Examples include VerificationMachine and IPCMachine.
 * These components do not maintain their own execution threads but instead
 * execute their logic within the context of the MessageBroker callback.
 */
class ReactiveMachine : public BaseMachine
{
  public:
    /**
     * As long as this class owns no resources, its designed minimal form keeps
     * inheriting constructors (AUTOSAR A12-1-6) and relies on
     * implicitly generated copy/move operations.
     */
    using BaseMachine::BaseMachine;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_COMMON_MACHINES_REACTIVE_MACHINE_H
