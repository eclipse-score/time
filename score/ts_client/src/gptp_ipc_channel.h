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
#ifndef SCORE_TS_CLIENT_SRC_GPTP_IPC_CHANNEL_H
#define SCORE_TS_CLIENT_SRC_GPTP_IPC_CHANNEL_H

#include "score/ts_client/src/gptp_ipc_data.h"

#include <atomic>
#include <cstdint>

namespace score
{
namespace ts
{
namespace details
{

/// Default shared memory name for the gPTP IPC channel.
constexpr char kGptpIpcName[] = "/gptp_ptp_info";

/// Magic number to validate the shared memory region ('GPTP').
inline constexpr std::uint32_t kGptpIpcMagic = 0x47505450U;

/**
 * @brief Shared memory layout for gPTP IPC (seqlock protocol).
 *
 * Single-writer (TimeSlave), multi-reader (TimeDaemon via ShmPTPEngine).
 * Aligned to 64 bytes (cache line) to avoid false sharing.
 *
 * Seqlock protocol:
 *  - Writer: seq++ (odd = writing), write data, seq_confirm = seq (even = readable)
 *  - Reader: read seq, read data, read seq_confirm; retry if seq != seq_confirm or odd
 */
struct alignas(64) GptpIpcRegion
{
    std::atomic<std::uint32_t> magic{kGptpIpcMagic};
    std::atomic<std::uint32_t> seq{0};
    score::ts::GptpIpcData data{};
    std::atomic<std::uint32_t> seq_confirm{1};
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TS_CLIENT_SRC_GPTP_IPC_CHANNEL_H
