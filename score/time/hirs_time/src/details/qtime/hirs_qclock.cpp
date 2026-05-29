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
#include "score/time/hirs_time/src/details/qtime/hirs_qclock.h"

#include "score/time/hirs_time/src/details/qtime/tick_provider.h"
#include "score/time/hirs_time/src/hirs_time.h"
#include "score/time/hirs_time/src/hirs_clock.h"

#include "score/lib/os/qnx/neutrino.h"

#include <chrono>
#include <memory>

namespace score
{
namespace time
{
namespace hirs_time
{
namespace qtime
{

ClockSnapshot<HirsTime::Timepoint, NoStatus> HirsQClock::Now() const noexcept
{
    const HirsTime::Timepoint tp{
        ClockCyclesToNanoseconds(score::os::qnx::Neutrino::instance().ClockCycles())};
    return ClockSnapshot<HirsTime::Timepoint, NoStatus>{tp, NoStatus{}};
}

std::chrono::nanoseconds HirsQClock::ClockCyclesToNanoseconds(
    const std::uint64_t clock_cycles) const noexcept
{
    const std::uint64_t cycles_per_sec = GetClockCyclesPerSec();
    std::chrono::nanoseconds converted{0};

    if (cycles_per_sec > 0U)
    {
        constexpr auto     billion    = static_cast<std::uint64_t>(std::nano::den);
        constexpr auto     max_value  = static_cast<std::uint64_t>(std::chrono::nanoseconds::max().count());
        constexpr std::uint64_t upper_bound{max_value / billion};
        const std::uint64_t     division_result{clock_cycles / cycles_per_sec};
        const std::uint64_t     division_rest{clock_cycles % cycles_per_sec};
        if ((division_result <= upper_bound) && (division_rest <= upper_bound))
        {
            converted = std::chrono::nanoseconds{division_result * billion +
                                                 division_rest * billion / cycles_per_sec};
        }
    }

    return converted;
}

}  // namespace qtime
}  // namespace hirs_time

template <>
std::shared_ptr<HirsClockBackend> detail::CreateBackend<HirsTime>()
{
    return std::make_shared<hirs_time::qtime::HirsQClock>();
}

}  // namespace time
}  // namespace score
