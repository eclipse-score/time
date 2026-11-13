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
#ifndef SCORE_TIME_HIGH_RES_STEADY_TIME_SRC_DETAILS_QTIME_HIGH_RES_STEADY_QCLOCK_H
#define SCORE_TIME_HIGH_RES_STEADY_TIME_SRC_DETAILS_QTIME_HIGH_RES_STEADY_QCLOCK_H

#include "score/time/clock/src/no_status.h"
#include "score/time/high_res_steady_time/src/high_res_steady_clock_backend.h"

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace qtime
{

/// @brief QNX production backend for HighResSteadyTime.
///
/// Reads @c CLOCK_MONOTONIC, the same clock the time slave uses to stamp Sync frames
/// (@c local_time). @c Neutrino::ClockCycles() is deliberately not used: it has a different epoch.
class HighResSteadyQClock final : public HighResSteadyClockBackend
{
  public:
    HighResSteadyQClock() noexcept = default;
    HighResSteadyQClock(const HighResSteadyQClock&) noexcept = delete;
    HighResSteadyQClock& operator=(const HighResSteadyQClock&) = delete;
    HighResSteadyQClock(HighResSteadyQClock&&) noexcept = delete;
    HighResSteadyQClock& operator=(HighResSteadyQClock&&) = delete;
    ~HighResSteadyQClock() noexcept override = default;

    /// @brief Returns the current HIRS snapshot from @c CLOCK_MONOTONIC.
    /// @return The current HIRS clock snapshot.
    ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus> Now() const noexcept override;
};

}  // namespace qtime
}  // namespace high_res_steady_time
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIGH_RES_STEADY_TIME_SRC_DETAILS_QTIME_HIGH_RES_STEADY_QCLOCK_H
