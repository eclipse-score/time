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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_PLATFORM_QNX_TIMESTAMP_SELECT_H
#define SCORE_TIME_SLAVE_SRC_GPTP_PLATFORM_QNX_TIMESTAMP_SELECT_H

#include <time.h>

namespace score::ts::details::qnx
{

/// @brief Sentinel returned by @c SelectTimestamp() when no timestamp from the locked
/// clock source could be determined.
constexpr ::timespec kNoTimestamp{-1, 0};

/// @brief Picks the timestamp for one frame given the per-interface source lock.
///
/// When @p hw_ts_supported is false the interface is software-locked and @p mono_fallback
/// is always returned. Otherwise @p bpf_ts is preferred, then @p ptp_get_time_ts, else
/// @c kNoTimestamp.
constexpr ::timespec SelectTimestamp(bool hw_ts_supported,
                                     bool bpf_ts_valid,
                                     ::timespec bpf_ts,
                                     bool ptp_get_time_valid,
                                     ::timespec ptp_get_time_ts,
                                     ::timespec mono_fallback) noexcept
{
    if (!hw_ts_supported)
        return mono_fallback;
    if (bpf_ts_valid)
        return bpf_ts;
    if (ptp_get_time_valid)
        return ptp_get_time_ts;
    return kNoTimestamp;
}

}  // namespace score::ts::details::qnx

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_PLATFORM_QNX_TIMESTAMP_SELECT_H
