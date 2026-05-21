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
#ifndef SCORE_TIMEDAEMON_CODE_IPC_RECEIVER_H
#define SCORE_TIMEDAEMON_CODE_IPC_RECEIVER_H

#include <optional>

namespace score
{
namespace td
{

///
/// \brief Receiver class to read data through ipc from Publisher
///
template <typename T>
class Receiver
{
  public:
    Receiver() = default;
    Receiver(const Receiver&) = default;
    Receiver& operator=(const Receiver&) = default;
    Receiver(Receiver&&) = default;
    Receiver& operator=(Receiver&&) = default;
    virtual ~Receiver() = default;

    ///
    /// \brief As long as it uses IPC handler below, it need to be explicitly initalized before use
    ///
    virtual bool Init() noexcept = 0;

    ///
    /// \brief method Update read data from ipc
    /// \return optional Data: optional is set when read of specific data is successful
    ///
    virtual std::optional<T> Receive() noexcept = 0;
};

}  // namespace td
}  // namespace score

#endif  // #ifndef SCORE_TIMEDAEMON_CODE_IPC_RECEIVER_H
