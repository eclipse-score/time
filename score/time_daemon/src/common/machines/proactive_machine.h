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
#ifndef SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_PROACTIVE_MACHINE_H
#define SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_PROACTIVE_MACHINE_H

#include "score/concurrency/condition_variable.h"
#include "score/time_daemon/src/common/machines/base_machine.h"

#include <score/jthread.hpp>
#include <chrono>
#include <mutex>

namespace score
{
namespace td
{

/**
 * @brief Base class for machine components that actively drive system behavior.
 *
 * ProactiveMachine serves as the foundation for components that have their own
 * execution threads and actively generate data or control system behavior rather
 * than just responding to events.
 */
class ProactiveMachine : public BaseMachine
{
  public:
    /**
     * As long as this class owns no resources, its designed minimal form keeps
     * inheriting constructors (AUTOSAR A12-1-6) and relies on
     * implicitly generated copy/move operations.
     */
    using BaseMachine::BaseMachine;

    /**
     * @brief Starts the machine's execution thread.
     *
     * This method initializes and starts the worker thread that drives
     * the machine's behavior.
     *
     * @return true if started successfully, false otherwise
     */
    virtual void Start() noexcept = 0;

    /**
     * @brief Stops the machine's execution thread.
     *
     * This method signals the worker thread to stop and waits for it
     * to terminate properly.
     *
     * @return true if stopped successfully, false otherwise
     */
    virtual void Stop() noexcept = 0;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_PROACTIVE_MACHINE_H
