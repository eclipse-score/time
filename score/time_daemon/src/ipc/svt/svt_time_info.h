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
#ifndef SCORE_TIME_DAEMON_SRC_IPC_SVT_SVT_TIME_INFO_H
#define SCORE_TIME_DAEMON_SRC_IPC_SVT_SVT_TIME_INFO_H

#include <cstdint>
#include <ostream>
#include <type_traits>

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/ipc/data_converter.h"

namespace score
{
namespace td
{
namespace svt
{

// NOTE: TimeBaseStatus, SyncFupSnapshot, and PDelayDataSnapshot intentionally
// mirror the fields of PtpStatus, SyncFupData, and PDelayData from ptp_time_info.h.
// The duplication is deliberate: ptp_time_info.h is the internal processing-domain
// type; svt_time_info.h is the IPC serialisation type. Keeping them separate avoids
// coupling the processing domain to the IPC layer's memory layout and allows each
// to evolve independently.

/**
 * \brief POD struct to hold PTP Status content
 * Until the field is public, the following values are considered:
 * - is_synchronized: true if the clock is synchronized, false otherwise
 * - is_timeout: true if a timeout occurred in current frame, false otherwise
 * - is_time_jump_future: true if a future time jump was detected in current frame, false otherwise
 * - is_time_jump_past: true if a past time jump was detected in current frame, false otherwise
 * - is_correct: true if the PTP status is correct, false otherwise
 */
struct TimeBaseStatus
{
    bool is_synchronized;
    bool is_timeout;
    bool is_time_jump_future;
    bool is_time_jump_past;
    bool is_correct;
};

/// \brief POD struct to hold PTP sync data content
struct SyncFupSnapshot
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
struct PDelayDataSnapshot
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
struct TimeBaseSnapshot
{
    uint64_t ptp_assumed_time;
    uint64_t local_time;
    double rate_deviation;
    TimeBaseStatus status;
    SyncFupSnapshot sync_fup_data;
    PDelayDataSnapshot pdelay_data;

    void CreateFrom(const PtpTimeInfo& info);
};

bool operator==(const TimeBaseSnapshot& ipcdata, const PtpTimeInfo& data) noexcept;
bool operator!=(const TimeBaseSnapshot& ipcdata, const PtpTimeInfo& data) noexcept;

/// \brief Comparing operators:
bool operator==(const TimeBaseStatus& first, const TimeBaseStatus& second) noexcept;
bool operator==(const SyncFupSnapshot& first, const SyncFupSnapshot& second) noexcept;
bool operator!=(const SyncFupSnapshot& first, const SyncFupSnapshot& second) noexcept;
bool operator==(const PDelayDataSnapshot& first, const PDelayDataSnapshot& second) noexcept;
bool operator!=(const PDelayDataSnapshot& first, const PDelayDataSnapshot& second) noexcept;
bool operator==(const TimeBaseSnapshot& first, const TimeBaseSnapshot& second) noexcept;
bool operator!=(const TimeBaseSnapshot& first, const TimeBaseSnapshot& second) noexcept;

/// \brief PrintTo and stream operators:

template <typename OutputStream>
inline auto& PrintTo(const TimeBaseStatus& status, OutputStream& os)
{
    return os << "Status: [" << status.is_synchronized << "|" << status.is_timeout << "|" << status.is_time_jump_future
              << "|" << status.is_time_jump_past << "|" << status.is_correct << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const TimeBaseStatus& status)
{
    return PrintTo(status, os);
}

template <typename OutputStream>
inline auto& PrintTo(const SyncFupSnapshot& data, OutputStream& os)
{
    return os << "SyncFupSnapshot:" << "[" << data.precise_origin_timestamp << "|" << data.reference_global_timestamp
              << "|" << data.reference_local_timestamp << "|" << data.sync_ingress_timestamp << "|"
              << data.correction_field << "|" << data.sequence_id << "|" << data.pdelay << "|" << data.port_number
              << "|" << data.clock_identity << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const SyncFupSnapshot& data)
{
    return PrintTo(data, os);
}

template <typename OutputStream>
inline auto& PrintTo(const PDelayDataSnapshot& data, OutputStream& os)
{
    return os << "PDelayDataSnapshot:" << "[" << data.request_origin_timestamp << "|" << data.request_receipt_timestamp
              << "|" << data.response_origin_timestamp << "|" << data.response_receipt_timestamp << "|"
              << data.reference_global_timestamp << "|" << data.reference_local_timestamp << "|" << data.sequence_id
              << "|" << data.pdelay << "|" << data.req_port_number << "|" << data.req_clock_identity << "|"
              << data.resp_port_number << "|" << data.resp_clock_identity << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const PDelayDataSnapshot& data)
{
    return PrintTo(data, os);
}

template <typename OutputStream>
inline auto& PrintTo(const TimeBaseSnapshot& info, OutputStream& os)
{
    return os << "[" << info.ptp_assumed_time << "|" << info.local_time << "|" << info.status << "|"
              << info.sync_fup_data << "|" << info.pdelay_data << "]";
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const TimeBaseSnapshot& info)
{
    return PrintTo(info, os);
}

/// \brief  gtest compatibility:
void PrintTo(const TimeBaseStatus& status, std::ostream* os);
void PrintTo(const SyncFupSnapshot& data, std::ostream* os);
void PrintTo(const PDelayDataSnapshot& data, std::ostream* os);
void PrintTo(const TimeBaseSnapshot& info, std::ostream* os);

}  // namespace svt

/**
 * \brief DataConverter specialization for TimeBaseSnapshot
 */
template <>
struct DataConverter<PtpTimeInfo, svt::TimeBaseSnapshot>
{
    static svt::TimeBaseSnapshot Convert(const PtpTimeInfo& src)
    {
        svt::TimeBaseSnapshot dst{};
        dst.CreateFrom(src);
        return dst;
    }
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_IPC_SVT_SVT_TIME_INFO_H
