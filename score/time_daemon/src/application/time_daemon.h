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

#include "score/time_daemon/src/application/timebase_handler.h"
#include "score/time_daemon/src/application/timebase_handler_factory.h"

#include "score/mw/lifecycle/application.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace score
{
namespace td
{

/// @brief Orchestrates the timebase handlers: initializes, polls and stops them.
///
/// Owns the handlers. The default constructor takes them from CreateTimebaseHandlers(). It is defined inline, so
/// that only executables which use it have to link a provider of that function.
///
/// @see TimebaseHandler
/// @see score::mw::lifecycle::Application
class TimeDaemon final : public score::mw::lifecycle::Application
{
  public:
    explicit TimeDaemon() : TimeDaemon{CreateTimebaseHandlers()} {}
    explicit TimeDaemon(std::vector<std::unique_ptr<TimebaseHandler>> timebase_handlers);
    ~TimeDaemon() noexcept override = default;

    TimeDaemon(TimeDaemon&&) noexcept = delete;
    TimeDaemon(const TimeDaemon&) noexcept = delete;
    TimeDaemon& operator=(TimeDaemon&&) & noexcept = delete;
    TimeDaemon& operator=(const TimeDaemon&) & noexcept = delete;

    /// @brief Initializes the handlers in vector order.
    ///
    /// Fails before initializing any handler if there is none, or if one of them is null.
    ///
    /// @param context Application context from the middleware.
    /// @return 0 on success, non-zero on failure.
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& context) override;

    /// @brief Polls the handlers until stop is requested, then stops them in reverse order.
    ///
    /// Each pass calls RunOnce() of every handler in vector order, then waits 100 ms.
    /// Requires a successful Initialize().
    ///
    /// @param token Stop token for graceful shutdown.
    /// @return 0 on success, non-zero on failure.
    std::int32_t Run(const score::cpp::stop_token& token) override;

  private:
    std::vector<std::unique_ptr<TimebaseHandler>> timebase_handlers_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_TIME_DAEMON_H
