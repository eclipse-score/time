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
#ifndef SCORE_TIME_CLOCK_SRC_INITIALIZATION_HOOK_H
#define SCORE_TIME_CLOCK_SRC_INITIALIZATION_HOOK_H

#include <type_traits>

namespace score
{
namespace time
{

/// @brief SFINAE gate for @c Clock<Tag>::Init().
///
/// The primary template is intentionally undefined. Clock domains whose backends
/// are always ready (e.g. @c HighResSteadyTime, @c std::chrono::steady_clock) must NOT
/// provide a specialization — calling @c Init() on such clocks is a compile error
/// (use of incomplete type).
///
/// Clock domains whose backends require explicit initialization (e.g. @c VehicleTime)
/// must provide a full explicit specialization supplying:
///   - @c static bool CallInit(Backend&) noexcept
///
/// @c CallInit receives a non-const reference to the backend because initialization
/// is a mutating operation.
///
/// @tparam Tag  Clock domain tag struct (e.g. VehicleTime).
template <typename Tag>
struct InitializationHook;

/// @brief Detects whether @c InitializationHook<Tag> has been specialised.
///
/// @c HasInitialization<Tag>::value is @c true only for clock domains that require
/// explicit initialization (e.g. VehicleTime). For always-ready clocks (HighResSteadyTime,
/// std::chrono::steady_clock, std::chrono::system_clock) the hook is undefined and
/// this trait is @c false.
template <typename Tag, typename = void>
struct HasInitialization : std::false_type
{
};

template <typename Tag>
struct HasInitialization<Tag,
                         std::void_t<decltype(&InitializationHook<Tag>::CallInit)>>
    : std::true_type
{
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_SRC_INITIALIZATION_HOOK_H
