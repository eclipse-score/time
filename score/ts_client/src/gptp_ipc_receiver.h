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
#ifndef SCORE_TS_CLIENT_SRC_GPTP_IPC_RECEIVER_H
#define SCORE_TS_CLIENT_SRC_GPTP_IPC_RECEIVER_H

#include "score/ts_client/src/gptp_ipc_channel.h"

#include <optional>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/**
 * @brief Multi-reader receiver for the gPTP IPC channel.
 *
 * Opens an existing POSIX shared memory segment (read-only) and reads
 * PtpTimeInfo using the seqlock protocol.  Used by ShmPTPEngine.
 */
class GptpIpcReceiver final
{
  public:
    GptpIpcReceiver() = default;
    ~GptpIpcReceiver();

    GptpIpcReceiver(const GptpIpcReceiver&) = delete;
    GptpIpcReceiver& operator=(const GptpIpcReceiver&) = delete;

    /// Open and map the shared memory segment (read-only).
    /// @return true on success.
    bool Init(const std::string& ipc_name = kGptpIpcName);

    /// Read a GptpIpcData snapshot using seqlock (up to 20 retries).
    /// @return The data if consistent, or std::nullopt on contention failure.
    std::optional<score::ts::GptpIpcData> Receive();

    /// Unmap the shared memory segment.
    void Close();

  private:
    const GptpIpcRegion* region_{nullptr};
    int shm_fd_{-1};
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TS_CLIENT_SRC_GPTP_IPC_RECEIVER_H
