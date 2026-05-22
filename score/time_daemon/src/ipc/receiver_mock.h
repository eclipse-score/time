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
#ifndef SCORE_TIME_DAEMON_SRC_IPC_RECEIVER_MOCK_H
#define SCORE_TIME_DAEMON_SRC_IPC_RECEIVER_MOCK_H

#include "score/time_daemon/src/ipc/receiver.h"

#include <gmock/gmock.h>

namespace score
{
namespace td
{

///
/// \brief Receiver class gmock
///
template <typename T>
class ReceiverMock : public Receiver<T>
{
  public:
    ReceiverMock() = default;
    ~ReceiverMock() override = default;

    MOCK_METHOD(bool, Init, (), (noexcept, override));
    MOCK_METHOD(std::optional<T>, Receive, (), (noexcept, override));
};

}  // namespace td
}  // namespace score

#endif  // #ifndef SCORE_TIME_DAEMON_SRC_IPC_RECEIVER_MOCK_H
