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
#ifndef EXAMPLES_TIME_HIGH_RES_STEADY_TIME_HIGH_RES_STEADY_TIME_HANDLER_H
#define EXAMPLES_TIME_HIGH_RES_STEADY_TIME_HIGH_RES_STEADY_TIME_HANDLER_H

#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

#include <cstdint>

namespace examples
{
namespace time
{
namespace high_res_steady_time
{

/// @brief A snapshot of the time report produced by HighResSteadyTimeHandler.
struct TimeReport
{
    /// @brief Current HIRS monotonic time in nanoseconds since an unspecified epoch.
    std::int64_t time_ns{0};
};

/// @brief Convenience wrapper that reads HighResSteadyClock in one call.
///
/// @par Testing pattern
/// @code
///   auto mock = std::make_shared<score::time::HighResSteadyClockBackendMock>();
///   score::time::test_utils::ScopedClockOverride<score::time::HighResSteadyTime> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
///   HighResSteadyTimeHandler handler;
///   const auto report = handler.GetCurrentTime();
/// @endcode
class HighResSteadyTimeHandler
{
  public:
    HighResSteadyTimeHandler()  = default;
    ~HighResSteadyTimeHandler() = default;

    HighResSteadyTimeHandler(const HighResSteadyTimeHandler&)             = delete;
    HighResSteadyTimeHandler& operator=(const HighResSteadyTimeHandler&)  = delete;
    HighResSteadyTimeHandler(HighResSteadyTimeHandler&&)                  = delete;
    HighResSteadyTimeHandler& operator=(HighResSteadyTimeHandler&&)       = delete;

    /// @brief Reads the current HIRS time and returns a report.
    [[nodiscard]] TimeReport GetCurrentTime() const noexcept
    {
        const auto snapshot = score::time::HighResSteadyClock::GetInstance().Now();
        return TimeReport{snapshot.TimePointNs().count()};
    }
};

}  // namespace high_res_steady_time
}  // namespace time
}  // namespace examples

#endif  // EXAMPLES_TIME_HIGH_RES_STEADY_TIME_HIGH_RES_STEADY_TIME_HANDLER_H
