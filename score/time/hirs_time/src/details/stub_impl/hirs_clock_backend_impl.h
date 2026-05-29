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
#ifndef SCORE_TIME_HIRS_TIME_SRC_DETAILS_STUB_IMPL_HIRS_CLOCK_BACKEND_IMPL_H
#define SCORE_TIME_HIRS_TIME_SRC_DETAILS_STUB_IMPL_HIRS_CLOCK_BACKEND_IMPL_H

// Internal header — include ONLY from stub_impl/hirs_clock_backend_impl.cpp.

#include "score/time/hirs_time/src/hirs_clock_backend.h"
#include "score/time/clock/src/no_status.h"

#include <chrono>

namespace score
{
namespace time
{
namespace detail
{

/// @brief Stub backend for the HIRS clock domain (host/test-only).
///
/// Wraps std::chrono::steady_clock so that Clock<HirsTime>::GetInstance()
/// resolves at link time even when ScopedClockOverride is not active.
class HirsClockBackendImpl final : public HirsClockBackend
{
  public:
    HirsClockBackendImpl() noexcept                           = default;
    ~HirsClockBackendImpl() noexcept override                 = default;
    HirsClockBackendImpl(const HirsClockBackendImpl&)                = delete;
    HirsClockBackendImpl& operator=(const HirsClockBackendImpl&)     = delete;
    HirsClockBackendImpl(HirsClockBackendImpl&&)                     = delete;
    HirsClockBackendImpl& operator=(HirsClockBackendImpl&&)          = delete;

    ClockSnapshot<HirsTime::Timepoint, NoStatus> Now() const noexcept override
    {
        const auto raw = std::chrono::steady_clock::now().time_since_epoch();
        const HirsTime::Timepoint tp{std::chrono::duration_cast<HirsTime::Duration>(raw)};
        return ClockSnapshot<HirsTime::Timepoint, NoStatus>{tp, NoStatus{}};
    }
};

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIRS_TIME_SRC_DETAILS_STUB_IMPL_HIRS_CLOCK_BACKEND_IMPL_H
