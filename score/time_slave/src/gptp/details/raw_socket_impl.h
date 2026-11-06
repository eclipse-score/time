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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_RAW_SOCKET_IMPL_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_RAW_SOCKET_IMPL_H

#include "score/time_slave/src/gptp/details/os_syscalls.h"
#include "score/time_slave/src/gptp/details/raw_socket.h"

#include <time.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/**
 * @brief Platform raw socket for Ethernet I/O with hardware or software timestamping.
 *
 * Implements platform-specific raw socket operations:
 * - Linux: AF_PACKET with SO_TIMESTAMPING for NIC-level timestamps
 * - QNX: io-pkt raw-socket shim
 */
class RawSocketImpl : public RawSocket
{
  public:
    /// @param sys  Optional syscall shim for unit testing.  nullptr → real OS calls.
    explicit RawSocketImpl(OsSyscalls* sys = nullptr) noexcept;
    ~RawSocketImpl() override;

    RawSocketImpl(const RawSocketImpl&) = delete;
    RawSocketImpl& operator=(const RawSocketImpl&) = delete;
    RawSocketImpl(RawSocketImpl&&) = delete;
    RawSocketImpl& operator=(RawSocketImpl&&) = delete;

    /// Open the socket bound to @p iface. Returns false on failure.
    bool Open(const std::string& iface) override;

    /// Configure hardware TX/RX timestamping on the already-opened socket and request kernel software
    /// receive timestamps as a fallback.
    ///
    /// On Linux, requests SO_TIMESTAMPING (NIC-level and kernel software timestamps). Returns false and
    /// logs a warning only if timestamping cannot be enabled at all.
    /// The socket continues to work, populating rx_ts with software timestamps
    /// (higher jitter but protocol correctness is unaffected).
    ///
    /// Returns false on failure. A no-op on platforms that don't support it.
    bool EnableTimestamping() override;

    /// Close the socket and release the file descriptor.
    void Close() override;

    /// Receive one frame.
    ///
    /// @param buf       Output buffer.
    /// @param buf_len   Capacity of @p buf.
    /// @param rx_ts     Output: hardware, else kernel software (CLOCK_MONOTONIC) receive timestamp
    ///                  (zeroed if unavailable).
    /// @param timeout_ms  <0 block indefinitely, 0 non-blocking, >0 timeout in ms.
    /// @return Number of bytes received, 0 on timeout, -1 on error.
    int Recv(std::uint8_t* buf, std::size_t buf_len, ::timespec& rx_ts, int timeout_ms) override;

    /// Send one frame.
    ///
    /// @param buf       Frame data including Ethernet header.
    /// @param len       Number of bytes to send.
    /// @param tx_ts      Output: hardware, else kernel software (CLOCK_MONOTONIC) transmit timestamp
    ///                   (zeroed if unavailable).
    /// @return Number of bytes sent, or -1 on error.
    int Send(const void* buf, int len, ::timespec& tx_ts) override;

    /// Return the underlying file descriptor (for advanced use / polling).
    int GetFd() const override
    {
        return fd_.load(std::memory_order_relaxed);
    }

  private:
    OsSyscalls* sys_{nullptr};
    std::atomic<int> fd_{-1};
    std::string iface_{};
    // Send() is called from the Rx and Pdelay threads; the TX timestamp error queue must not be shared between them.
    std::mutex send_mutex_{};
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_RAW_SOCKET_IMPL_H
