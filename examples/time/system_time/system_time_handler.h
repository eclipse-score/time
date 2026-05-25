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
#ifndef EXAMPLES_TIME_SYSTEM_TIME_SYSTEM_TIME_HANDLER_H
#define EXAMPLES_TIME_SYSTEM_TIME_SYSTEM_TIME_HANDLER_H

#include "score/time/system_time/system_clock.h"

#include <cstdint>

namespace examples
{
namespace time
{
namespace system_time
{

/// @brief A snapshot of the time report produced by SystemTimeHandler.
struct TimeReport
{
    /// @brief Current wall-clock (Unix epoch) time in nanoseconds.
    std::int64_t unix_ns{0};
};

/// @brief Convenience wrapper that reads SystemClock in one call.
///
/// @par Testing pattern
/// @code
///   auto mock = std::make_shared<score::time::SystemClockMock>();
///   score::time::test_utils::ScopedClockOverride<std::chrono::system_clock> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
///   SystemTimeHandler handler;
///   const auto report = handler.GetCurrentTime();
/// @endcode
class SystemTimeHandler
{
  public:
    SystemTimeHandler()  = default;
    ~SystemTimeHandler() = default;

    SystemTimeHandler(const SystemTimeHandler&)             = delete;
    SystemTimeHandler& operator=(const SystemTimeHandler&)  = delete;
    SystemTimeHandler(SystemTimeHandler&&)                  = delete;
    SystemTimeHandler& operator=(SystemTimeHandler&&)       = delete;

    /// @brief Reads the current system (wall-clock) time and returns a report.
    [[nodiscard]] TimeReport GetCurrentTime() const noexcept
    {
        const auto snapshot = score::time::SystemClock::GetInstance().Now();
        return TimeReport{snapshot.TimePointNs().count()};
    }
};

}  // namespace system_time
}  // namespace time
}  // namespace examples

#endif  // EXAMPLES_TIME_SYSTEM_TIME_SYSTEM_TIME_HANDLER_H
