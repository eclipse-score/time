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
#ifndef SCORE_TIME_DAEMON_SRC_IPC_CORE_PUBLISHER_IMPL_H
#define SCORE_TIME_DAEMON_SRC_IPC_CORE_PUBLISHER_IMPL_H

#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/logging_contexts.h"
#include "score/time_daemon/src/common/machines/reactive_machine.h"

#include "score/time_daemon/src/ipc/core/shared_memory_handler.h"
#include "score/time_daemon/src/ipc/data_converter.h"

namespace score
{
namespace td
{

///
/// \brief Publisher class to publish data through ipc to Client
///
template <typename DataType, typename IpcDataType>
class PublisherImpl : public ReactiveMachine, public Consumer<DataType>
{
  public:
    PublisherImpl(const std::string& name, const std::string& shared_memory_path) noexcept
        : ReactiveMachine(name), Consumer<DataType>(), shm_handler_{shared_memory_path}
    {
    }

    PublisherImpl(const PublisherImpl&) = delete;
    PublisherImpl& operator=(const PublisherImpl&) = delete;
    PublisherImpl(PublisherImpl&&) = delete;
    PublisherImpl& operator=(PublisherImpl&&) = delete;
    ~PublisherImpl() override = default;

    bool Init() override;
    void OnMessage(DataType data) override;

  private:
    SharedMemoryHandler<IpcDataType> shm_handler_;
};

template <typename DataType, typename IpcDataType>
bool PublisherImpl<DataType, IpcDataType>::Init()
{
    return shm_handler_.Init();
}

template <typename DataType, typename IpcDataType>
void PublisherImpl<DataType, IpcDataType>::OnMessage(DataType data)
{
    const IpcDataType ipc_data = ConvertToIpcData<IpcDataType>(data);
    shm_handler_.Send(ipc_data);
}

}  // namespace td
}  // namespace score

#endif  // #ifndef SCORE_TIME_DAEMON_SRC_IPC_CORE_PUBLISHER_IMPL_H
