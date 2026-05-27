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
#ifndef SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_EVENT_DRIVEN_MACHINE_H
#define SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_EVENT_DRIVEN_MACHINE_H

#include "score/time_daemon/src/common/machines/proactive_machine.h"
#include "score/concurrency/condition_variable.h"

#include <score/jthread.hpp>
#include <chrono>
#include <mutex>

namespace score
{
namespace td
{

/**
 * @brief Event-driven machine that responds to external events or timeout conditions.
 *
 * EventDrivenMachine extends ProactiveMachine to provide a reactive execution model
 * where the machine waits for external events and responds accordingly. It operates
 * on a timeout-based mechanism where it can either process events when they occur
 * or handle timeout conditions when no events arrive within the specified duration.
 */
class EventDrivenMachine : public ProactiveMachine
{
  public:
    /**
     * @brief Constructs an EventDrivenMachine with a specified name and timeout duration.
     *
     * This constructor initializes the EventDrivenMachine with a unique name and
     * a timeout duration that determines how long the machine waits for events
     * before triggering a timeout condition.
     *
     * @param name The name of the machine instance.
     * @param timeout The timeout duration in milliseconds for waiting for events.
     */
    explicit EventDrivenMachine(const std::string& name, const std::chrono::milliseconds timeout);

    EventDrivenMachine(const EventDrivenMachine&) = delete;
    EventDrivenMachine& operator=(const EventDrivenMachine&) = delete;
    EventDrivenMachine(EventDrivenMachine&&) noexcept = delete;
    EventDrivenMachine& operator=(EventDrivenMachine&&) noexcept = delete;

    ~EventDrivenMachine() override = default;

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

    /**
     * @brief Triggers the worker to process an event immediately.
     *
     * Call this when an event occurs (e.g., new message received).
     */
    void NotifyEvent() noexcept;

  protected:
    /**
     * @brief Called when an event is triggered before timeout.
     */
    virtual void OnEvent() noexcept = 0;

    /**
     * @brief Called when timeout expires without an event.
     */
    virtual void OnTimeout() noexcept = 0;

  private:
    void WorkerFunction(const score::cpp::stop_token& stop_token) noexcept;

    std::mutex cv_mutex_;
    score::concurrency::InterruptibleConditionalVariable cv_;
    score::cpp::jthread worker_;

    const std::chrono::milliseconds kTimeout_;
    bool event_pending_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_EVENT_DRIVEN_MACHINE_H
