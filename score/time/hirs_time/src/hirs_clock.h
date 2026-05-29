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
#ifndef SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_H
#define SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_H

#include "score/time/hirs_time/src/hirs_time.h"
#include "score/time/clock/src/clock.h"
#include "score/time/clock/src/clock_snapshot.h"
#include "score/time/clock/src/no_status.h"


namespace score
{
namespace time
{

class HirsClockBackend;

template <>
struct ClockTraits<HirsTime>
{
    using Backend        = HirsClockBackend;
    using Duration       = HirsTime::Duration;
    using Timepoint      = HirsTime::Timepoint;
    using Snapshot       = ClockSnapshot<Timepoint, NoStatus>;

    /// \brief Obtains the current HIRS clock snapshot from the backend.
    static Snapshot CallNow(const Backend& impl) noexcept;
};

using HirsClock = Clock<HirsTime>;
using HirsTimePoint = ClockTraits<HirsTime>::Timepoint;
using HirsSnapshot = ClockTraits<HirsTime>::Snapshot;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_H
