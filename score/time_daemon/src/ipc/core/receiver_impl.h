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
#ifndef SCORE_TIME_DAEMON_SRC_IPC_CORE_RECEIVER_IMPL_H
#define SCORE_TIME_DAEMON_SRC_IPC_CORE_RECEIVER_IMPL_H

#include "score/time_daemon/src/ipc/receiver.h"

#include "score/time_daemon/src/ipc/core/shared_memory_handler.h"

namespace score
{
namespace td
{

///
/// \brief Receiver impl class to receive data from ipc
///
template <typename IpcDataType>
class ReceiverImpl : public Receiver<IpcDataType>
{
  public:
    ReceiverImpl(const std::string& shared_memory_path) noexcept
        : Receiver<IpcDataType>(), shm_handler_{shared_memory_path}
    {
    }

    ReceiverImpl(const ReceiverImpl&) = delete;
    ReceiverImpl& operator=(const ReceiverImpl&) = delete;
    ReceiverImpl(ReceiverImpl&&) = delete;
    ReceiverImpl& operator=(ReceiverImpl&&) = delete;
    ~ReceiverImpl() override = default;

    bool Init() noexcept override;

    std::optional<IpcDataType> Receive() noexcept override;

  private:
    SharedMemoryHandler<IpcDataType> shm_handler_;
};

template <typename IpcDataType>
bool ReceiverImpl<IpcDataType>::Init() noexcept
{
    return shm_handler_.Init();
}

template <typename IpcDataType>
std::optional<IpcDataType> ReceiverImpl<IpcDataType>::Receive() noexcept
{
    return shm_handler_.Receive();
}

}  // namespace td
}  // namespace score

#endif  // #ifndef SCORE_TIME_DAEMON_SRC_IPC_CORE_RECEIVER_IMPL_H
