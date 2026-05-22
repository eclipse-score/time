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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_CLOCK_UTIL_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_CLOCK_UTIL_H

#include "score/time_slave/src/gptp/details/ptp_types.h"

#include <time.h>
#include <cstdint>

namespace score
{
namespace ts
{
namespace details
{

inline std::int64_t MonoNs() noexcept
{
    ::timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return static_cast<std::int64_t>(ts.tv_sec) * kNsPerSec + ts.tv_nsec;
}

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_CLOCK_UTIL_H
