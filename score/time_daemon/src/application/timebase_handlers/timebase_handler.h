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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_TIMEBASE_HANDLER_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_TIMEBASE_HANDLER_H

#include <score/stop_token.hpp>
#include <future>

namespace score
{
namespace td
{

/// \brief Abstract base class to handle timebase operations
///
/// This class provides an interface for initializing, running, and stopping a
/// timebase handler. It is non-copyable and non-movable to ensure a single
/// instance manages the timebase.
class TimebaseHandler
{
  public:
    /// \brief Default constexpr constructor
    constexpr TimebaseHandler() noexcept = default;

    /// \brief Virtual destructor
    virtual ~TimebaseHandler() noexcept = default;

    /// \brief Deleted copy constructor to prevent copying
    TimebaseHandler(const TimebaseHandler&) = delete;

    /// \brief Deleted move constructor to prevent moving
    TimebaseHandler(TimebaseHandler&&) = delete;

    /// \brief Deleted copy assignment operator
    TimebaseHandler& operator=(const TimebaseHandler&) = delete;

    /// \brief Deleted move assignment operator
    TimebaseHandler& operator=(TimebaseHandler&&) = delete;

    /// \brief Status of the timebase handler
    enum class Status
    {
        kIdle = 0,  ///< Handler is idle, not initialized
        kStarting,  ///< Handler is starting
        kWorking,   ///< Handler is actively running
        kStopped    ///< Handler is stopped
    };

    /// \brief Initialize the timebase handler
    ///
    /// This function prepares the handler for operation. Must be called before
    /// RunOnce(). No exception should be thrown.
    virtual void Initialize() noexcept = 0;

    /// \brief Start the timebase handler
    ///
    /// Signals the handler to start its operation. Must be called after
    /// Initialize() and before RunOnce().
    virtual void Start(const score::cpp::stop_token& token) noexcept = 0;

    /// \brief Runs once the timebase handler main functionality
    ///
    /// Should be called periodically to invoke all poll based actions
    virtual void RunOnce() noexcept = 0;

    /// \brief Stop the timebase handler
    ///
    /// Signals the handler to stop its operation and clean up resources. Should
    /// return quickly and not throw exceptions.
    virtual void Stop() noexcept = 0;

    /// \brief Reports whether initialization failed
    ///
    /// Returns true once the handler gave up on its own, e.g. an initialization timeout.
    /// A stop requested from outside is not a failure. Call from the thread that calls RunOnce().
    virtual bool IsFailed() const noexcept = 0;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_TIMEBASE_HANDLER_H
