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
#include "score/time/high_res_steady_time/src/details/system_clock/high_res_steady_clock_backend_impl.h"

#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"
#include "score/time/high_res_steady_time/src/high_res_steady_time.h"

#include <time.h>
#include <chrono>
#include <memory>

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace sys_time
{

HighResSteadyClockBackendImpl::HighResSteadyClockBackendImpl() noexcept = default;
HighResSteadyClockBackendImpl::~HighResSteadyClockBackendImpl() noexcept = default;

ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus> HighResSteadyClockBackendImpl::Now() const noexcept
{
    // Must be CLOCK_MONOTONIC: the time slave stamps Sync frames with it and VehicleClock subtracts those
    // stamps from this clock's readings. std::chrono::high_resolution_clock is CLOCK_REALTIME on Linux.
    ::timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{HighResSteadyTime::Timepoint{}, NoStatus{}};
    }
    const std::chrono::nanoseconds raw{static_cast<std::int64_t>(ts.tv_sec) * std::nano::den + ts.tv_nsec};
    const HighResSteadyTime::Timepoint tp{std::chrono::duration_cast<HighResSteadyTime::Duration>(raw)};
    return ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{tp, NoStatus{}};
}

}  // namespace sys_time
}  // namespace high_res_steady_time

// req-Id: comp_req__time__supported_platforms
template <>
std::shared_ptr<HighResSteadyClockBackend> detail::CreateBackend<HighResSteadyTime>()
{
    return std::make_shared<high_res_steady_time::sys_time::HighResSteadyClockBackendImpl>();
}

}  // namespace time
}  // namespace score
