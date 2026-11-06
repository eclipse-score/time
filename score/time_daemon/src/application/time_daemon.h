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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_TIME_DAEMON_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_TIME_DAEMON_H

#include "score/time_daemon/src/application/timebase_handlers/timebase_handler.h"

#include "score/mw/lifecycle/application.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace score
{
namespace td
{

/// @brief Main entry point for the TimeDaemon process. Orchestrates the lifecycle
/// of all daemon components.
///
/// Owns the handlers. The default constructor uses the linked CreateTimebaseHandlers() implementation.
/// The alternative constructor accepts handlers directly; the daemon core still requires a factory at link time.
///
/// @see TimebaseHandler
/// @see score::mw::lifecycle::Application
class TimeDaemon final : public score::mw::lifecycle::Application
{
  public:
    explicit TimeDaemon();
    explicit TimeDaemon(std::vector<std::unique_ptr<TimebaseHandler>> timebase_handlers);
    ~TimeDaemon() noexcept override = default;

    TimeDaemon(TimeDaemon&&) noexcept = delete;
    TimeDaemon(const TimeDaemon&) noexcept = delete;
    TimeDaemon& operator=(TimeDaemon&&) & noexcept = delete;
    TimeDaemon& operator=(const TimeDaemon&) & noexcept = delete;

    /// @brief Initializes the handlers in list order.
    ///
    /// Wires all pub/sub relationships via MessageBroker topic configuration.
    ///
    /// @param context Application context from the middleware.
    /// @return 0 on success, non-zero on failure.
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& context) override;

    /// @brief Starts the ProactiveMachines and monitors the stop token.
    ///
    /// Execution loop monitors stop_token. On termination request, stops all
    /// ProactiveMachines in reverse order of startup for clean shutdown.
    ///
    /// @param token Stop token for graceful shutdown.
    /// @return 0 on success, non-zero on failure.
    std::int32_t Run(const score::cpp::stop_token& token) override;

  private:
    void InitializeAllTimebases();
    void StartAllTimebases(const score::cpp::stop_token& token);
    void IterateAllTimebases();
    void StopAllTimebases();

    std::vector<std::unique_ptr<TimebaseHandler>> timebase_handlers_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_TIME_DAEMON_H
