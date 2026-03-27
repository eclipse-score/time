/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#ifndef SCORE_TIMESLAVE_CODE_GPTP_DETAILS_RAW_SOCKET_H
#define SCORE_TIMESLAVE_CODE_GPTP_DETAILS_RAW_SOCKET_H

#include "score/TimeSlave/code/gptp/details/i_raw_socket.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <time.h>

namespace score
{
namespace ts
{
namespace details
{

/**
 * @brief Platform raw socket for Ethernet I/O with hardware timestamping.
 *
 * On Linux uses AF_PACKET / SO_TIMESTAMPING.
 * On QNX uses the QNX raw-socket shim.
 */
class RawSocket : public IRawSocket
{
  public:
    RawSocket() noexcept = default;
    ~RawSocket() override;

    RawSocket(const RawSocket&)            = delete;
    RawSocket& operator=(const RawSocket&) = delete;
    RawSocket(RawSocket&&)                 = delete;
    RawSocket& operator=(RawSocket&&)      = delete;

    /// Open the socket bound to @p iface. Returns false on failure.
    bool Open(const std::string& iface) override;

    /// Configure hardware TX/RX timestamping on the already-opened socket.
    /// Returns false on failure. A no-op on platforms that don't support it.
    bool EnableHwTimestamping() override;

    /// Close the socket and release the file descriptor.
    void Close() override;

    /// Receive one frame.
    ///
    /// @param buf       Output buffer.
    /// @param buf_len   Capacity of @p buf.
    /// @param hwts      Output: hardware receive timestamp (zeroed if unavailable).
    /// @param timeout_ms  <0 block indefinitely, 0 non-blocking, >0 timeout in ms.
    /// @return Number of bytes received, 0 on timeout, -1 on error.
    int Recv(std::uint8_t* buf, std::size_t buf_len,
             ::timespec& hwts, int timeout_ms) override;

    /// Send one frame.
    ///
    /// @param buf       Frame data including Ethernet header.
    /// @param len       Number of bytes to send.
    /// @param hwts      Output: hardware transmit timestamp (zeroed if unavailable).
    /// @return Number of bytes sent, or -1 on error.
    int Send(const void* buf, int len, ::timespec& hwts) override;

    /// Return the underlying file descriptor (for advanced use / polling).
    int GetFd() const override { return fd_; }

  private:
    int         fd_{-1};
    std::string iface_{};
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIMESLAVE_CODE_GPTP_DETAILS_RAW_SOCKET_H
