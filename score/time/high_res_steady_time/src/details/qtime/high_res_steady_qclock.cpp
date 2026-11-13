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
#include "score/time/high_res_steady_time/src/details/qtime/high_res_steady_qclock.h"

#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"
#include "score/time/high_res_steady_time/src/high_res_steady_time.h"

#include <time.h>
#include <chrono>
#include <cstdint>
#include <memory>

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace qtime
{

ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus> HighResSteadyQClock::Now() const noexcept
{
    // Must be CLOCK_MONOTONIC: the time slave stamps Sync frames with it and VehicleClock subtracts those
    // stamps from this clock's readings. ClockCycles() has a different epoch (GH #310).
    ::timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{}, NoStatus{}};
    }
    const std::chrono::nanoseconds raw{static_cast<std::int64_t>(ts.tv_sec) * std::nano::den + ts.tv_nsec};
    const HighResSteadyTime::Timepoint tp{std::chrono::duration_cast<HighResSteadyTime::Duration>(raw)};
    return ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{tp, NoStatus{}};
}

}  // namespace qtime
}  // namespace high_res_steady_time

// req-Id: comp_req__time__supported_platforms
template <>
std::shared_ptr<HighResSteadyClockBackend> detail::CreateBackend<HighResSteadyTime>()
{
    return std::make_shared<high_res_steady_time::qtime::HighResSteadyQClock>();
}

}  // namespace time
}  // namespace score
