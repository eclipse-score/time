/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#pragma once

#include <cstdint>
#include <optional>
#include <time.h>

namespace score_time::utils
{

    /**
     * @brief Safely read clock time with error checking
     * @param clk Clock ID (CLOCK_MONOTONIC, CLOCK_REALTIME, etc.)
     * @return Nanoseconds since epoch, or nullopt on error
     */
    [[nodiscard]] inline std::optional<std::int64_t> ClockNsSafe(clockid_t clk) noexcept
    {
        ::timespec ts{};
        if (::clock_gettime(clk, &ts) != 0)
        {
            return std::nullopt;
        }

        // Check for overflow: tv_sec * 1e9 must fit in int64_t
        // Maximum safe value: INT64_MAX / 1e9 = ~9223372036 seconds (~292 years)
        constexpr std::int64_t kMaxSafeSeconds = 9223372036LL;
        if (ts.tv_sec > kMaxSafeSeconds || ts.tv_sec < -kMaxSafeSeconds)
        {
            return std::nullopt;
        }

        return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL +
               static_cast<std::int64_t>(ts.tv_nsec);
    }

    /**
     * @brief Read clock time, returning 0 on error (backward-compatible API)
     * @param clk Clock ID
     * @return Nanoseconds since epoch, or 0 on error
     */
    inline std::int64_t ClockNs(clockid_t clk) noexcept
    {
        return ClockNsSafe(clk).value_or(0);
    }

} // namespace score_time::utils
