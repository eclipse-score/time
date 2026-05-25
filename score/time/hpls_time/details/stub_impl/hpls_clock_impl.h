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
#ifndef SCORE_TIME_HPLS_TIME_DETAILS_STUB_IMPL_HPLS_CLOCK_IMPL_H
#define SCORE_TIME_HPLS_TIME_DETAILS_STUB_IMPL_HPLS_CLOCK_IMPL_H

// Internal header — include ONLY from stub_impl/hpls_clock_impl.cpp.

#include "score/time/hpls_time/hpls_clock_iface.h"
#include "score/time/clock/no_status.h"

#include <chrono>

namespace score
{
namespace time
{
namespace detail
{

/// @brief Stub backend for the HPLS clock domain (host/test-only).
///
/// Wraps std::chrono::steady_clock so that Clock<HplsTime>::GetInstance()
/// resolves at link time even when ScopedClockOverride is not active.
class HplsClockImpl final : public HplsClockIface
{
  public:
    HplsClockImpl() noexcept                           = default;
    ~HplsClockImpl() noexcept override                 = default;
    HplsClockImpl(const HplsClockImpl&)                = delete;
    HplsClockImpl& operator=(const HplsClockImpl&)     = delete;
    HplsClockImpl(HplsClockImpl&&)                     = delete;
    HplsClockImpl& operator=(HplsClockImpl&&)          = delete;

    ClockSnapshot<HplsTime::Timepoint, NoStatus> Now() const noexcept override
    {
        const auto raw = std::chrono::steady_clock::now().time_since_epoch();
        const HplsTime::Timepoint tp{std::chrono::duration_cast<HplsTime::Duration>(raw)};
        return ClockSnapshot<HplsTime::Timepoint, NoStatus>{tp, NoStatus{}};
    }
};

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HPLS_TIME_DETAILS_STUB_IMPL_HPLS_CLOCK_IMPL_H
