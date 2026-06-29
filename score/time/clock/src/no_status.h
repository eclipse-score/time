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

#ifndef SCORE_TIME_CLOCK_SRC_NO_STATUS_H
#define SCORE_TIME_CLOCK_SRC_NO_STATUS_H

namespace score
{
namespace time
{

/// @brief Empty placeholder status type for clocks that have no quality concept
///        (e.g. HighResSteadyClock, std::chrono::steady_clock).
///
/// Used as the @c StatusT parameter of @c ClockSnapshot when the clock does not
/// produce a synchronisation-quality indicator alongside its time-point.
struct NoStatus
{
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_SRC_NO_STATUS_H
