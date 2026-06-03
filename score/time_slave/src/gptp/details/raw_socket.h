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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_RAW_SOCKET_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_RAW_SOCKET_H

#include <time.h>
#include <cstddef>
#include <cstdint>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/// Interface for a platform raw socket used by GptpEngine and PeerDelayMeasurer.
class RawSocket
{
  public:
    virtual ~RawSocket() noexcept = default;

    /// Open the socket bound to @p iface. Returns false on failure.
    virtual bool Open(const std::string& iface) = 0;

    /// Configure hardware TX/RX timestamping. Returns false on failure.
    virtual bool EnableHwTimestamping() = 0;

    /// Close the socket and release the file descriptor.
    virtual void Close() = 0;

    /// Receive one frame.
    /// @return Number of bytes received, 0 on timeout, -1 on error.
    virtual int Recv(std::uint8_t* buf, std::size_t buf_len, ::timespec& hwts, int timeout_ms) = 0;

    /// Send one frame.
    /// @return Number of bytes sent, or -1 on error.
    virtual int Send(const void* buf, int len, ::timespec& hwts) = 0;

    /// Return the underlying file descriptor.
    virtual int GetFd() const = 0;
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_RAW_SOCKET_H
