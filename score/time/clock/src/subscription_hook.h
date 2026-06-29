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
#ifndef SCORE_TIME_CLOCK_SRC_SUBSCRIPTION_HOOK_H
#define SCORE_TIME_CLOCK_SRC_SUBSCRIPTION_HOOK_H

namespace score
{
namespace time
{

/// @brief SFINAE gate for @c Clock<Tag>::Subscribe<EventType>() and @c Unsubscribe<EventType>().
///
/// The primary template is intentionally undefined. Attempting to subscribe to an
/// EventType for which no specialization exists produces a compile error
/// (use of incomplete type).
///
/// Each specialization must provide:
///   - @c Callback        — the callable type accepted by @c Subscribe()
///   - @c static void Subscribe(Backend&, Callback)
///   - @c static void Unsubscribe(Backend&)
///
/// @tparam Tag        The clock domain tag (e.g. VehicleTime).
/// @tparam EventType  The event struct type (e.g. TimeSlaveSyncData<VehicleTime>).
template <typename Tag, typename EventType>
struct SubscriptionHook;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_SRC_SUBSCRIPTION_HOOK_H
