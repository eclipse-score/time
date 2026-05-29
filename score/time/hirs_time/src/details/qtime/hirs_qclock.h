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
#ifndef SCORE_TIME_HIRS_TIME_SRC_DETAILS_QTIME_HIRS_QCLOCK_H
#define SCORE_TIME_HIRS_TIME_SRC_DETAILS_QTIME_HIRS_QCLOCK_H

#include "score/time/hirs_time/src/hirs_clock_backend.h"
#include "score/time/clock/src/no_status.h"

#include <cstdint>

namespace score
{
namespace time
{
namespace hirs_time
{
namespace qtime
{

///
/// \brief QNX production backend for HirsTime.
///
/// Reads the current time from the QNX hardware clock via @c Neutrino::ClockCycles()
/// and converts the raw cycle count to nanoseconds using @c GetClockCyclesPerSec().
///
class HirsQClock final : public HirsClockBackend
{
  public:
    HirsQClock() noexcept                          = default;
    HirsQClock(const HirsQClock&) noexcept         = delete;
    HirsQClock& operator=(const HirsQClock&)       = delete;
    HirsQClock(HirsQClock&&) noexcept              = delete;
    HirsQClock& operator=(HirsQClock&&)            = delete;
    ~HirsQClock() noexcept override                = default;

    /// \brief Returns the current HIRS snapshot from the QNX hardware clock.
    ClockSnapshot<HirsTime::Timepoint, NoStatus> Now() const noexcept override;

  private:
    /// \brief Converts raw hardware clock cycles to nanoseconds.
    std::chrono::nanoseconds ClockCyclesToNanoseconds(std::uint64_t clock_cycles) const noexcept;
};

}  // namespace qtime
}  // namespace hirs_time
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIRS_TIME_SRC_DETAILS_QTIME_HIRS_QCLOCK_H
