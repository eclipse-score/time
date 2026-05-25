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
#ifndef EXAMPLES_TIME_STEADY_TIME_STEADY_TIME_HANDLER_H
#define EXAMPLES_TIME_STEADY_TIME_STEADY_TIME_HANDLER_H

#include "score/time/steady_time/steady_clock.h"

#include <cstdint>

namespace examples
{
namespace time
{
namespace steady_time
{

/// @brief A snapshot of the time report produced by SteadyTimeHandler.
struct TimeReport
{
    /// @brief Current monotonic time in nanoseconds since an unspecified epoch (typically boot).
    std::int64_t monotonic_ns{0};
};

/// @brief Convenience wrapper that reads SteadyClock in one call.
///
/// @par Testing pattern
/// @code
///   auto mock = std::make_shared<score::time::SteadyClockMock>();
///   score::time::test_utils::ScopedClockOverride<std::chrono::steady_clock> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
///   SteadyTimeHandler handler;
///   const auto report = handler.GetCurrentTime();
/// @endcode
class SteadyTimeHandler
{
  public:
    SteadyTimeHandler()  = default;
    ~SteadyTimeHandler() = default;

    SteadyTimeHandler(const SteadyTimeHandler&)             = delete;
    SteadyTimeHandler& operator=(const SteadyTimeHandler&)  = delete;
    SteadyTimeHandler(SteadyTimeHandler&&)                  = delete;
    SteadyTimeHandler& operator=(SteadyTimeHandler&&)       = delete;

    /// @brief Reads the current steady time and returns a report.
    [[nodiscard]] TimeReport GetCurrentTime() const noexcept
    {
        const auto snapshot = score::time::SteadyClock::GetInstance().Now();
        return TimeReport{snapshot.TimePointNs().count()};
    }
};

}  // namespace steady_time
}  // namespace time
}  // namespace examples

#endif  // EXAMPLES_TIME_STEADY_TIME_STEADY_TIME_HANDLER_H
