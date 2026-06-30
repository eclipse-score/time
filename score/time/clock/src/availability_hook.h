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
#ifndef SCORE_TIME_CLOCK_SRC_AVAILABILITY_HOOK_H
#define SCORE_TIME_CLOCK_SRC_AVAILABILITY_HOOK_H

#include <type_traits>

namespace score
{
namespace time
{

/// @brief SFINAE gate for @c Clock<Tag>::IsAvailable() and @c WaitUntilAvailable().
///
/// The primary template is intentionally undefined. Clock domains that are always
/// available (e.g. @c HighResSteadyTime, @c std::chrono::steady_clock) must NOT provide a
/// specialization — attempting to call @c IsAvailable() / @c WaitUntilAvailable()
/// on such clocks is a compile error (use of incomplete type).
///
/// Clock domains that require a readiness check (e.g. @c VehicleTime, whose backend
/// opens shared memory) must provide a full explicit specialization supplying:
///   - @c static bool CallIsAvailable(const Backend&) noexcept
///   - @c static bool CallWaitUntilAvailable(const Backend&,
///                       const score::cpp::stop_token&,
///                       std::chrono::steady_clock::time_point) noexcept
///
/// @tparam Tag  Clock domain tag struct (e.g. VehicleTime).
template <typename Tag>
struct AvailabilityHook;

/// @brief Detects whether @c AvailabilityHook<Tag> has been specialised.
///
/// @c HasAvailability<Tag>::value is @c true only for clock domains that require a
/// readiness check (e.g. VehicleTime). For always-available clocks (HighResSteadyTime,
/// std::chrono::steady_clock, std::chrono::system_clock) the hook is undefined and
/// this trait is @c false.
template <typename Tag, typename = void>
struct HasAvailability : std::false_type
{
};

template <typename Tag>
struct HasAvailability<Tag,
                       std::void_t<decltype(&AvailabilityHook<Tag>::CallIsAvailable)>>
    : std::true_type
{
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_SRC_AVAILABILITY_HOOK_H
