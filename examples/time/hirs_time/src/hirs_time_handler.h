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
#ifndef EXAMPLES_TIME_HIRS_TIME_HIRS_TIME_HANDLER_H
#define EXAMPLES_TIME_HIRS_TIME_HIRS_TIME_HANDLER_H

#include "score/time/hirs_time/src/hirs_clock.h"

#include <cstdint>

namespace examples
{
namespace time
{
namespace hirs_time
{

/// @brief A snapshot of the time report produced by HirsTimeHandler.
struct TimeReport
{
    /// @brief Current HIRS monotonic time in nanoseconds since an unspecified epoch.
    std::int64_t time_ns{0};
};

/// @brief Convenience wrapper that reads HirsClock in one call.
///
/// @par Testing pattern
/// @code
///   auto mock = std::make_shared<score::time::HirsClockBackendMock>();
///   score::time::test_utils::ScopedClockOverride<score::time::HirsTime> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
///   HirsTimeHandler handler;
///   const auto report = handler.GetCurrentTime();
/// @endcode
class HirsTimeHandler
{
  public:
    HirsTimeHandler()  = default;
    ~HirsTimeHandler() = default;

    HirsTimeHandler(const HirsTimeHandler&)             = delete;
    HirsTimeHandler& operator=(const HirsTimeHandler&)  = delete;
    HirsTimeHandler(HirsTimeHandler&&)                  = delete;
    HirsTimeHandler& operator=(HirsTimeHandler&&)       = delete;

    /// @brief Reads the current HIRS time and returns a report.
    [[nodiscard]] TimeReport GetCurrentTime() const noexcept
    {
        const auto snapshot = score::time::HirsClock::GetInstance().Now();
        return TimeReport{snapshot.TimePointNs().count()};
    }
};

}  // namespace hirs_time
}  // namespace time
}  // namespace examples

#endif  // EXAMPLES_TIME_HIRS_TIME_HIRS_TIME_HANDLER_H
