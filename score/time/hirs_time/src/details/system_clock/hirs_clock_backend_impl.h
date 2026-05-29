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
#ifndef SCORE_TIME_HIRS_TIME_SRC_DETAILS_SYSTEM_CLOCK_HIRS_CLOCK_BACKEND_IMPL_H
#define SCORE_TIME_HIRS_TIME_SRC_DETAILS_SYSTEM_CLOCK_HIRS_CLOCK_BACKEND_IMPL_H

#include "score/time/hirs_time/src/hirs_clock_backend.h"
#include "score/time/clock/src/no_status.h"

namespace score
{
namespace time
{
namespace hirs_time
{
namespace sys_time
{

///
/// \brief Production HirsTime backend for non-QNX platforms.
///
/// Reads the current time from @c std::chrono::high_resolution_clock and
/// converts it to an @c HirsTime::Timepoint.
///
class HirsClockBackendImpl final : public HirsClockBackend
{
  public:
    HirsClockBackendImpl() noexcept;
    HirsClockBackendImpl(const HirsClockBackendImpl&) noexcept            = delete;
    HirsClockBackendImpl& operator=(const HirsClockBackendImpl&) noexcept = delete;
    HirsClockBackendImpl(HirsClockBackendImpl&&) noexcept                 = delete;
    HirsClockBackendImpl& operator=(HirsClockBackendImpl&&) noexcept      = delete;
    ~HirsClockBackendImpl() noexcept override;

    /// \brief Returns the current HIRS snapshot using @c high_resolution_clock.
    ClockSnapshot<HirsTime::Timepoint, NoStatus> Now() const noexcept override;
};

}  // namespace sys_time
}  // namespace hirs_time
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIRS_TIME_SRC_DETAILS_SYSTEM_CLOCK_HIRS_CLOCK_BACKEND_IMPL_H
