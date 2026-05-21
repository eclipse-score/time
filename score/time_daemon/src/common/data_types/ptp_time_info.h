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
#ifndef SCORE_TIMEDAEMON_CODE_COMMON_DATA_TYPES_PTP_TIME_INFO_H
#define SCORE_TIMEDAEMON_CODE_COMMON_DATA_TYPES_PTP_TIME_INFO_H

#include <cstdint>
#include <ostream>
#include <type_traits>

#include "score/time/HighPrecisionLocalSteadyClock/high_precision_local_steady_clock.h"

namespace score
{
namespace td
{

/**
 * \brief POD struct to hold PTP Status content
 * Until the field is public, the following values are considered:
 * - is_synchronized: true if the clock is synchronized, false otherwise
 * - is_timeout: true if a timeout occurred in current frame, false otherwise
 * - is_time_jump_future: true if a future time jump was detected in current frame, false otherwise
 * - is_time_jump_past: true if a past time jump was detected in current frame, false otherwise
 * - is_correct: true if the PTP status is correct, false otherwise
 */
struct PtpStatus
{
    bool is_synchronized;
    bool is_timeout;
    bool is_time_jump_future;
    bool is_time_jump_past;
    bool is_correct;
};

/// \brief POD struct to hold PTP sync data content
struct SyncFupData
{
    std::uint64_t precise_origin_timestamp;
    std::uint64_t reference_global_timestamp;
    std::uint64_t reference_local_timestamp;
    std::uint64_t sync_ingress_timestamp;
    std::uint64_t correction_field;
    std::uint16_t sequence_id;
    std::uint64_t pdelay;
    std::uint32_t port_number;
    std::uint64_t clock_identity;
};

/// \brief POD struct to hold PTP Pdelay measurement result content
struct PDelayData
{
    std::uint64_t request_origin_timestamp;
    std::uint64_t request_receipt_timestamp;
    std::uint64_t response_origin_timestamp;
    std::uint64_t response_receipt_timestamp;
    std::uint64_t reference_global_timestamp;
    std::uint64_t reference_local_timestamp;
    std::uint16_t sequence_id;
    std::uint64_t pdelay;
    std::uint32_t req_port_number;
    std::uint64_t req_clock_identity;
    std::uint32_t resp_port_number;
    std::uint64_t resp_clock_identity;
};

/// \brief General type class to store and pass all necessary data
struct PtpTimeInfo
{
    /**
     * The local time base, used in the ptp evaluations
     */
    using ReferenceClock = score::time::HighPrecisionLocalSteadyClock;

    std::chrono::nanoseconds ptp_assumed_time;
    ReferenceClock::time_point local_time;
    double rate_deviation;
    PtpStatus status;
    SyncFupData sync_fup_data;
    PDelayData pdelay_data;
};

/// \brief Comparing operators:
bool operator==(const PtpStatus& first, const PtpStatus& second) noexcept;
bool operator==(const SyncFupData& first, const SyncFupData& second) noexcept;
bool operator!=(const SyncFupData& first, const SyncFupData& second) noexcept;
bool operator==(const PDelayData& first, const PDelayData& second) noexcept;
bool operator!=(const PDelayData& first, const PDelayData& second) noexcept;
bool operator==(const PtpTimeInfo& first, const PtpTimeInfo& second) noexcept;
bool operator!=(const PtpTimeInfo& first, const PtpTimeInfo& second) noexcept;

/// \brief PrintTo and stream operators:

template <typename OutputStream>
inline auto& PrintTo(const PtpStatus& status, OutputStream& os)
{
    return os << "Status: [" << status.is_synchronized << "|" << status.is_timeout << "|" << status.is_time_jump_future
              << "|" << status.is_time_jump_past << "|" << status.is_correct << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const PtpStatus& status)
{
    return PrintTo(status, os);
}

template <typename OutputStream>
inline auto& PrintTo(const SyncFupData& data, OutputStream& os)
{
    return os << "SyncFupData:" << "[" << data.precise_origin_timestamp << "|" << data.reference_global_timestamp << "|"
              << data.reference_local_timestamp << "|" << data.sync_ingress_timestamp << "|" << data.correction_field
              << "|" << data.sequence_id << "|" << data.pdelay << "|" << data.port_number << "|" << data.clock_identity
              << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const SyncFupData& data)
{
    return PrintTo(data, os);
}

template <typename OutputStream>
inline auto& PrintTo(const PDelayData& data, OutputStream& os)
{
    return os << "PDelayData:" << "[" << data.request_origin_timestamp << "|" << data.request_receipt_timestamp << "|"
              << data.response_origin_timestamp << "|" << data.response_receipt_timestamp << "|"
              << data.reference_global_timestamp << "|" << data.reference_local_timestamp << "|" << data.sequence_id
              << "|" << data.pdelay << "|" << data.req_port_number << "|" << data.req_clock_identity << "|"
              << data.resp_port_number << "|" << data.resp_clock_identity << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const PDelayData& data)
{
    return PrintTo(data, os);
}

template <typename OutputStream>
inline auto& PrintTo(const PtpTimeInfo& info, OutputStream& os)
{
    return os << "[" << info.ptp_assumed_time.count() << "|" << info.local_time.time_since_epoch().count() << "|"
              << info.status << "|" << info.sync_fup_data << "|" << info.pdelay_data << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const PtpTimeInfo& info)
{
    return PrintTo(info, os);
}

/// \brief  gtest compatibility:
void PrintTo(const PtpStatus& status, std::ostream* os);
void PrintTo(const SyncFupData& data, std::ostream* os);
void PrintTo(const PDelayData& data, std::ostream* os);
void PrintTo(const PtpTimeInfo& info, std::ostream* os);

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_COMMON_DATA_TYPES_PTP_TIME_INFO_H
