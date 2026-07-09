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
#ifndef SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_PERIODIC_MACHINE_H
#define SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_PERIODIC_MACHINE_H

#include "score/concurrency/condition_variable.h"
#include "score/time_daemon/src/common/machines/proactive_machine.h"

#include <score/jthread.hpp>
#include <chrono>
#include <mutex>

namespace score
{
namespace td
{

/**
 * @brief A machine component that executes periodic tasks at fixed intervals.
 *
 * PeriodicMachine extends ProactiveMachine to provide a framework for components
 * that need to perform regular, time-based operations. It manages its own worker
 * thread that executes a periodic task at configurable intervals, making it ideal
 * for monitoring, data collection, or other cyclical operations.
 */
class PeriodicMachine : public ProactiveMachine
{
  public:
    /**
     * @brief Constructs a PeriodicMachine with the specified name and cycle interval.
     *
     * @param name The name of the machine instance
     * @param threadCycle The periodic execution interval in milliseconds
     */
    explicit PeriodicMachine(const std::string& name, const std::chrono::milliseconds threadCycle);

    PeriodicMachine(const PeriodicMachine&) = delete;
    PeriodicMachine& operator=(const PeriodicMachine&) = delete;
    PeriodicMachine(PeriodicMachine&&) noexcept = delete;
    PeriodicMachine& operator=(PeriodicMachine&&) noexcept = delete;

    ~PeriodicMachine() override = default;

    /**
     * @brief Starts the machine's execution thread.
     *
     * This method initializes and starts the worker thread that drives
     * the machine's behavior.
     *
     * @return true if started successfully, false otherwise
     */
    void Start() noexcept override;

    /**
     * @brief Stops the machine's execution thread.
     *
     * This method signals the worker thread to stop and waits for it
     * to terminate properly.
     *
     * Clients and/or inherited classes must call it in their destructor to ensure proper cleanup.
     *
     * @return true if stopped successfully, false otherwise
     */
    void Stop() noexcept override;

  protected:
    /**
     * @brief Task executed periodically at fixed intervals.
     *
     * Called every kCycleTime_ milliseconds.
     */
    virtual void PeriodicTask() noexcept = 0;

  private:
    void WorkerFunction(const score::cpp::stop_token& stop_token) noexcept;

    std::mutex cv_mutex_;
    score::concurrency::InterruptibleConditionalVariable cv_;
    score::cpp::jthread worker_;

    const std::chrono::milliseconds kCycleTime_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_PERIODIC_MACHINE_H
