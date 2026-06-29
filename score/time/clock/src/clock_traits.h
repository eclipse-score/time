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
#ifndef SCORE_TIME_CLOCK_SRC_CLOCK_TRAITS_H
#define SCORE_TIME_CLOCK_SRC_CLOCK_TRAITS_H

namespace score
{
namespace time
{

/// @brief Type and behaviour traits for a clock domain.
///
/// The primary template is intentionally incomplete. Every clock domain must provide
/// a full explicit specialization supplying at minimum:
///
///   - @c Backend          — abstract backend type (virtual interface)
///   - @c Duration         — std::chrono duration type
///   - @c Timepoint        — std::chrono::time_point<Tag, Duration>
///   - @c Snapshot         — ClockSnapshot<Timepoint, StatusT>
///   - @c CallNow(const Backend&) — obtains a snapshot
///
/// Readiness-check support is opt-in via @c AvailabilityHook<Tag> (see availability_hook.h).
/// Subscription support is opt-in via @c SubscriptionHook<Tag, EventType> (see subscription_hook.h).
/// Quality-status types (@c StatusFlag, @c Status) live on the tag struct itself (e.g. @c VehicleTime).
///
template <typename Tag>
struct ClockTraits;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_SRC_CLOCK_TRAITS_H
