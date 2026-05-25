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
#ifndef EXAMPLES_TIME_HPLS_TIME_HPLS_TIME_HANDLER_H
#define EXAMPLES_TIME_HPLS_TIME_HPLS_TIME_HANDLER_H

#include "score/time/hpls_time/hpls_clock.h"

#include <cstdint>

namespace examples
{
namespace time
{
namespace hpls_time
{

/// @brief A snapshot of the time report produced by HplsTimeHandler.
struct TimeReport
{
    /// @brief Current HPLS monotonic time in nanoseconds since an unspecified epoch.
    std::int64_t time_ns{0};
};

/// @brief Convenience wrapper that reads HplsClock in one call.
///
/// @par Testing pattern
/// @code
///   auto mock = std::make_shared<score::time::HplsClockMock>();
///   score::time::test_utils::ScopedClockOverride<score::time::HplsTime> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
///   HplsTimeHandler handler;
///   const auto report = handler.GetCurrentTime();
/// @endcode
class HplsTimeHandler
{
  public:
    HplsTimeHandler()  = default;
    ~HplsTimeHandler() = default;

    HplsTimeHandler(const HplsTimeHandler&)             = delete;
    HplsTimeHandler& operator=(const HplsTimeHandler&)  = delete;
    HplsTimeHandler(HplsTimeHandler&&)                  = delete;
    HplsTimeHandler& operator=(HplsTimeHandler&&)       = delete;

    /// @brief Reads the current HPLS time and returns a report.
    [[nodiscard]] TimeReport GetCurrentTime() const noexcept
    {
        const auto snapshot = score::time::HplsClock::GetInstance().Now();
        return TimeReport{snapshot.TimePointNs().count()};
    }
};

}  // namespace hpls_time
}  // namespace time
}  // namespace examples

#endif  // EXAMPLES_TIME_HPLS_TIME_HPLS_TIME_HANDLER_H
