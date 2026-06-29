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
#include "score/memory/shared/shared_memory_factory.h"

#include <memory>
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

    bool Open(const std::string& ipc_name = kGptpIpcName);
    std::optional<score::ts::GptpIpcData> Receive();
    void Close();

  private:
    const GptpIpcRegion* region_{nullptr};
    std::shared_ptr<score::memory::shared::ISharedMemoryResource> shm_resource_;
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TS_CLIENT_SRC_GPTP_IPC_RECEIVER_H
