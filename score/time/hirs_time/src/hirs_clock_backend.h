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
#ifndef SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_BACKEND_H
#define SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_BACKEND_H

#include "score/time/hirs_time/src/hirs_time.h"
#include "score/time/clock/src/clock_snapshot.h"
#include "score/time/clock/src/no_status.h"

namespace score
{
namespace time
{

///
/// \brief Pure-virtual pimpl interface for the HIRS time domain backend.
class HirsClockBackend
{
  public:
    virtual ~HirsClockBackend() noexcept = default;

    /// \brief Returns the current HIRS snapshot (time-point; status is NoStatus).
    virtual ClockSnapshot<HirsTime::Timepoint, NoStatus> Now() const noexcept = 0;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_BACKEND_H
