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
#ifndef SCORE_TS_CLIENT_SRC_GPTP_IPC_PUBLISHER_H
#define SCORE_TS_CLIENT_SRC_GPTP_IPC_PUBLISHER_H

#include "score/ts_client/src/gptp_ipc_channel.h"
#include "score/memory/shared/shared_memory_factory.h"

#include <memory>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/**
 * @brief Single-writer publisher for the gPTP IPC channel.
 *
 * Creates the POSIX shared memory segment and writes PtpTimeInfo using
 * the seqlock protocol.  Used by TimeSlave.
 */
class GptpIpcPublisher final
{
  public:
    GptpIpcPublisher() = default;
    ~GptpIpcPublisher();

    GptpIpcPublisher(const GptpIpcPublisher&) = delete;
    GptpIpcPublisher& operator=(const GptpIpcPublisher&) = delete;

    bool Open(const std::string& ipc_name = kGptpIpcName);
    void Publish(const score::ts::GptpIpcData& data);
    void Close();

  private:
    GptpIpcRegion* region_{nullptr};
    std::shared_ptr<score::memory::shared::ISharedMemoryResource> shm_resource_;
    std::string ipc_name_;
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TS_CLIENT_SRC_GPTP_IPC_PUBLISHER_H
