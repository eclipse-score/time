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
#include "score/time_slave/src/gptp/details/network_identity_impl.h"
#include "score/time_slave/src/gptp/details/raw_socket_impl.h"

#include <gtest/gtest.h>

#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/socket.h>
#include <time.h>
#include <cerrno>
#include <cstring>
#include <ctime>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

// ── FakeOsSyscalls ─────────────────────────────────────────────────────────────
//
// A controllable OsSyscalls implementation that never touches the real OS.
// Each method has configurable return values; side-effects (like filling cmsg
// data) are controlled by boolean flags.

class FakeOsSyscalls final : public OsSyscalls
{
  public:
    // ── socket ────────────────────────────────────────────────────────────────
    int socket_fd{42};        // fd returned on success
    bool socket_fail{false};  // true → return -1

    int socket_call(int /*domain*/, int /*type*/, int /*protocol*/) noexcept override
    {
        if (socket_fail)
        {
            errno = EPERM;
            return -1;
        }
        return socket_fd;
    }

    // ── ioctl ─────────────────────────────────────────────────────────────────
    bool ioctl_siocgifindex_fail{false};   // SIOCGIFINDEX failure
    bool ioctl_siocshwtstamp_fail{false};  // first SIOCSHWTSTAMP failure (fallback exercised)

    int ioctl_call(int /*fd*/, unsigned long req, void* /*arg*/) noexcept override
    {
        if (req == SIOCGIFINDEX)
            return ioctl_siocgifindex_fail ? -1 : 0;
        if (req == SIOCSHWTSTAMP)
        {
            if (ioctl_siocshwtstamp_fail)
            {
                ioctl_siocshwtstamp_fail = false;  // first call fails; second succeeds
                return -1;
            }
            return 0;
        }
        return 0;
    }

    // ── bind ─────────────────────────────────────────────────────────────────
    bool bind_fail{false};

    int bind_call(int /*fd*/, const ::sockaddr* /*addr*/, ::socklen_t /*addrlen*/) noexcept override
    {
        return bind_fail ? -1 : 0;
    }

    // ── setsockopt ────────────────────────────────────────────────────────────
    bool setsockopt_fail{false};

    int setsockopt_call(int /*fd*/,
                        int /*level*/,
                        int /*optname*/,
                        const void* /*optval*/,
                        ::socklen_t /*optlen*/) noexcept override
    {
        return setsockopt_fail ? -1 : 0;
    }

    // ── close ────────────────────────────────────────────────────────────────
    int close_count{0};
    int last_closed_fd{-1};

    int close_call(int fd) noexcept override
    {
        ++close_count;
        last_closed_fd = fd;
        return 0;
    }

    // ── poll ─────────────────────────────────────────────────────────────────
    int poll_result{0};  // 0=timeout, -1=error, >0=ready
    int poll_revents{POLLIN};

    int poll_call(::pollfd* fds, ::nfds_t /*nfds*/, int /*timeout*/) noexcept override
    {
        if (fds != nullptr)
            fds[0].revents = (poll_result > 0) ? static_cast<short>(poll_revents) : 0;
        return poll_result;
    }

    // ── recvmsg ───────────────────────────────────────────────────────────────
    // Regular (non-errqueue) recvmsg — used by Recv().
    ::ssize_t recvmsg_result{-1};   // bytes returned; -1 = error
    bool recvmsg_fill_hwts{false};  // fill SO_TIMESTAMPING cmsg with ts[2]={1,500000000}
    bool recvmsg_fill_swts{false};  // fill SO_TIMESTAMPING cmsg with ts[0]=recvmsg_swts (software, CLOCK_REALTIME)
    ::timespec recvmsg_swts{};

    // MSG_ERRQUEUE recvmsg — used by DrainErrQueue (before send) and TX timestamp (after send).
    // Sequence per Send() call:
    //   calls 0 .. errqueue_drain_count-1  → return 1 (drain loop body runs)
    //   call  errqueue_drain_count         → return -1 (terminates DrainErrQueue loop)
    //   call  errqueue_drain_count+1       → TX timestamp: fill tx_ts if tx_fill_hwts, return tx_result
    int errqueue_drain_count{0};  // how many drain entries to simulate
    bool tx_fill_hwts{false};     // fill SO_TIMESTAMPING cmsg in TX-timestamp recvmsg
    bool tx_fill_swts{false};     // same, but only ts[0]=tx_swts (software, CLOCK_REALTIME)
    ::timespec tx_swts{};
    ::ssize_t tx_result{14};  // TX-timestamp recvmsg return value

    int recvmsg_call_count{0};

    ::ssize_t recvmsg_call(int /*fd*/, ::msghdr* msg, int flags) noexcept override
    {
        ++recvmsg_call_count;

        if ((flags & MSG_ERRQUEUE) != 0)
        {
            const int idx = errqueue_call_count_++;
            if (idx < errqueue_drain_count)
                return 1;  // drain entry present — loop body exercises
            if (idx == errqueue_drain_count)
                return -1;  // end of drain queue → loop exits
            // idx > errqueue_drain_count: this is the TX-timestamp recvmsg
            if ((tx_fill_hwts || tx_fill_swts) && msg != nullptr && msg->msg_control != nullptr &&
                msg->msg_controllen >= CMSG_SPACE(3 * sizeof(::timespec)))
            {
                auto* cm = reinterpret_cast<::cmsghdr*>(msg->msg_control);
                cm->cmsg_level = SOL_SOCKET;
                cm->cmsg_type = SO_TIMESTAMPING;
                cm->cmsg_len = CMSG_LEN(3 * sizeof(::timespec));
                auto* ts = reinterpret_cast<::timespec*>(CMSG_DATA(cm));
                ts[0] = tx_fill_hwts ? ::timespec{0, 0} : tx_swts;
                ts[1] = {0, 0};
                ts[2] = tx_fill_hwts ? ::timespec{1, 500'000'000L} : ::timespec{0, 0};
                msg->msg_controllen = CMSG_SPACE(3 * sizeof(::timespec));
            }
            return tx_result;
        }

        // Regular recvmsg (from Recv()).
        if (recvmsg_result < 0)
            return -1;

        // Optionally inject SO_TIMESTAMPING cmsg for Recv() rx_ts extraction.
        if ((recvmsg_fill_hwts || recvmsg_fill_swts) && msg != nullptr && msg->msg_control != nullptr &&
            msg->msg_controllen >= CMSG_SPACE(3 * sizeof(::timespec)))
        {
            auto* cm = reinterpret_cast<::cmsghdr*>(msg->msg_control);
            cm->cmsg_level = SOL_SOCKET;
            cm->cmsg_type = SO_TIMESTAMPING;
            cm->cmsg_len = CMSG_LEN(3 * sizeof(::timespec));
            auto* ts = reinterpret_cast<::timespec*>(CMSG_DATA(cm));
            ts[0] = recvmsg_fill_hwts ? ::timespec{0, 0} : recvmsg_swts;
            ts[1] = {0, 0};
            ts[2] = recvmsg_fill_hwts ? ::timespec{1, 500'000'000L} : ::timespec{0, 0};
            msg->msg_controllen = CMSG_SPACE(3 * sizeof(::timespec));
        }

        if (msg != nullptr && msg->msg_iov != nullptr && recvmsg_result > 0)
        {
            const std::size_t n = std::min(static_cast<std::size_t>(recvmsg_result), msg->msg_iov[0].iov_len);
            std::memset(msg->msg_iov[0].iov_base, 0, n);
        }
        return recvmsg_result;
    }

  private:
    int errqueue_call_count_{0};  // counts MSG_ERRQUEUE recvmsg calls (resets per test instance)

  public:
    // ── send ──────────────────────────────────────────────────────────────────
    ::ssize_t send_result{14};  // bytes "sent"; -1 = error

    ::ssize_t send_call(int /*fd*/, const void* /*buf*/, ::size_t len, int /*flags*/) noexcept override
    {
        if (send_result < 0)
            return -1;
        return static_cast<::ssize_t>(len);
    }
};

// Helper: open the socket successfully using the given fake syscalls.
void OpenSocket(RawSocketImpl& sock, FakeOsSyscalls& /*fake*/)
{
    (void)sock.Open("eth0");
}

// ── Software-timestamp fallback helpers ──────────────────────────────────────

std::int64_t ToNs(const ::timespec& ts)
{
    return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec;
}

::timespec NowOn(clockid_t clk)
{
    ::timespec ts{};
    ::clock_gettime(clk, &ts);
    return ts;
}

::timespec RealtimeMinus(std::int64_t age_ns)
{
    const std::int64_t ns = ToNs(NowOn(CLOCK_REALTIME)) - age_ns;
    return ::timespec{static_cast<::time_t>(ns / 1'000'000'000LL), static_cast<long>(ns % 1'000'000'000LL)};
}

}  // namespace

// ── RawSocket — closed-state guard paths ──────────────────────────────────────

TEST(RawSocketTest, DefaultConstruct_GetFd_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    fake.socket_fail = true;  // never actually open
    RawSocketImpl sock{&fake};
    EXPECT_EQ(sock.GetFd(), -1);
}

TEST(RawSocketTest, Close_WhenNotOpen_IsNoOp)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    EXPECT_NO_THROW(sock.Close());
    EXPECT_EQ(sock.GetFd(), -1);
    EXPECT_EQ(fake.close_count, 0);  // no real fd was open
}

TEST(RawSocketTest, EnableTimestamping_WhenNotOpen_ReturnsFalse)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    EXPECT_FALSE(sock.EnableTimestamping());
}

TEST(RawSocketTest, Recv_WhenNotOpen_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    std::uint8_t buf[64] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 0), -1);
}

TEST(RawSocketTest, Recv_NullBuf_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(nullptr, 64U, rx_ts, 0), -1);
}

TEST(RawSocketTest, Recv_ZeroBufLen_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[1] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, 0U, rx_ts, 0), -1);
}

TEST(RawSocketTest, Send_WhenNotOpen_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    const std::uint8_t data[14] = {};
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(data, 14, tx_ts), -1);
}

TEST(RawSocketTest, Send_NullBuf_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(nullptr, 14, tx_ts), -1);
}

TEST(RawSocketTest, Send_ZeroLen_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    const std::uint8_t data[1] = {};
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(data, 0, tx_ts), -1);
}

TEST(RawSocketTest, Send_NegativeLen_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    const std::uint8_t data[1] = {};
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(data, -1, tx_ts), -1);
}

// ── RawSocket — Open() failure paths ─────────────────────────────────────────

TEST(RawSocketTest, Open_SocketCallFails_ReturnsFalse)
{
    FakeOsSyscalls fake;
    fake.socket_fail = true;
    RawSocketImpl sock{&fake};
    EXPECT_FALSE(sock.Open("eth0"));
    EXPECT_EQ(sock.GetFd(), -1);
}

TEST(RawSocketTest, Open_IoctlSiocgifindexFails_ReturnsFalse)
{
    FakeOsSyscalls fake;
    fake.ioctl_siocgifindex_fail = true;
    RawSocketImpl sock{&fake};
    EXPECT_FALSE(sock.Open("eth0"));
    EXPECT_EQ(sock.GetFd(), -1);
    // The fake fd must have been closed on failure
    EXPECT_EQ(fake.close_count, 1);
    EXPECT_EQ(fake.last_closed_fd, fake.socket_fd);
}

TEST(RawSocketTest, Open_BindFails_ReturnsFalse)
{
    FakeOsSyscalls fake;
    fake.bind_fail = true;
    RawSocketImpl sock{&fake};
    EXPECT_FALSE(sock.Open("eth0"));
    EXPECT_EQ(sock.GetFd(), -1);
    EXPECT_EQ(fake.close_count, 1);
}

TEST(RawSocketTest, Open_Success_ReturnsTrueAndStoresFd)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    EXPECT_TRUE(sock.Open("eth0"));
    EXPECT_EQ(sock.GetFd(), fake.socket_fd);
}

TEST(RawSocketTest, Open_NonExistentInterface_ReturnsFalse)
{
    // Uses RealOsSyscalls; ioctl(SIOCGIFINDEX) will fail for unknown iface.
    RawSocketImpl sock;
    EXPECT_FALSE(sock.Open("nonexistent_eth_zzz"));
}

TEST(RawSocketTest, Open_NonExistentInterface_GetFdRemainsNegativeOne)
{
    RawSocketImpl sock;
    (void)sock.Open("nonexistent_eth_zzz");
    EXPECT_EQ(sock.GetFd(), -1);
}

// ── RawSocket — Close() ───────────────────────────────────────────────────────

TEST(RawSocketTest, Close_AfterOpen_CallsCloseOnFd)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    EXPECT_EQ(sock.GetFd(), fake.socket_fd);
    sock.Close();
    EXPECT_EQ(sock.GetFd(), -1);
    EXPECT_EQ(fake.close_count, 1);
    EXPECT_EQ(fake.last_closed_fd, fake.socket_fd);
}

TEST(RawSocketTest, Close_CalledTwiceAfterOpen_IsIdempotent)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    sock.Close();
    EXPECT_NO_THROW(sock.Close());
    EXPECT_EQ(sock.GetFd(), -1);
    EXPECT_EQ(fake.close_count, 1);  // second Close() is a no-op
}

TEST(RawSocketTest, Destructor_AfterOpen_ClosesSocket)
{
    FakeOsSyscalls fake;
    {
        RawSocketImpl sock{&fake};
        OpenSocket(sock, fake);
        EXPECT_EQ(sock.GetFd(), fake.socket_fd);
    }  // destructor calls Close()
    EXPECT_EQ(fake.close_count, 1);
}

// ── RawSocket — EnableTimestamping() ───────────────────────────────────────────────────────────────────────────────

TEST(RawSocketTest, EnableTimestamping_Success_ReturnsTrue)
{
    FakeOsSyscalls fake;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    EXPECT_TRUE(sock.EnableTimestamping());
}

TEST(RawSocketTest, EnableTimestamping_SiocshwtstampFallback_StillReturnsTrue)
{
    // First SIOCSHWTSTAMP ioctl fails → fallback (second call) is attempted.
    FakeOsSyscalls fake;
    fake.ioctl_siocshwtstamp_fail = true;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    EXPECT_TRUE(sock.EnableTimestamping());
}

TEST(RawSocketTest, EnableTimestamping_SetsockoptFails_ReturnsFalse)
{
    FakeOsSyscalls fake;
    fake.setsockopt_fail = true;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    EXPECT_FALSE(sock.EnableTimestamping());
}

// ── RawSocket — Recv() ────────────────────────────────────────────────────────

TEST(RawSocketTest, Recv_PollTimeout_ReturnsZero)
{
    FakeOsSyscalls fake;
    fake.poll_result = 0;  // timeout
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[64] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), 0);
}

TEST(RawSocketTest, Recv_PollError_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    fake.poll_result = -1;  // poll error
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[64] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), -1);
}

TEST(RawSocketTest, Recv_RecvmsgFails_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    fake.poll_result = 1;
    fake.recvmsg_result = -1;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[64] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), -1);
}

TEST(RawSocketTest, Recv_Success_NoTimestamp_ReturnsLen)
{
    FakeOsSyscalls fake;
    fake.poll_result = 1;
    fake.recvmsg_result = 14;  // 14 bytes received
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[256] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), 14);
    EXPECT_EQ(rx_ts.tv_sec, 0);
    EXPECT_EQ(rx_ts.tv_nsec, 0);
}

TEST(RawSocketTest, Recv_WithSoTimestampingCmsg_ExtractsHwts)
{
    FakeOsSyscalls fake;
    fake.poll_result = 1;
    fake.recvmsg_result = 14;
    fake.recvmsg_fill_hwts = true;  // inject ts[2]={1, 500_000_000}
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[256] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), 14);
    EXPECT_EQ(rx_ts.tv_sec, 1);
    EXPECT_EQ(rx_ts.tv_nsec, 500'000'000L);
}

TEST(RawSocketTest, Recv_HardwareTimestampPreferredOverSoftware)
{
    FakeOsSyscalls fake;
    fake.poll_result = 1;
    fake.recvmsg_result = 14;
    fake.recvmsg_fill_hwts = true;
    fake.recvmsg_fill_swts = true;
    fake.recvmsg_swts = RealtimeMinus(0);
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[256] = {};
    ::timespec rx_ts{};
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), 14);
    EXPECT_EQ(rx_ts.tv_sec, 1);
    EXPECT_EQ(rx_ts.tv_nsec, 500'000'000L);
}

TEST(RawSocketTest, Recv_SoftwareTimestamp_ConvertedToMonotonicWithPacketAge)
{
    constexpr std::int64_t kAgeNs = 5'000'000LL;  // packet was stamped 5 ms before Recv() reads it
    FakeOsSyscalls fake;
    fake.poll_result = 1;
    fake.recvmsg_result = 14;
    fake.recvmsg_fill_swts = true;
    fake.recvmsg_swts = RealtimeMinus(kAgeNs);
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    std::uint8_t buf[256] = {};
    ::timespec rx_ts{};
    const std::int64_t before = ToNs(NowOn(CLOCK_MONOTONIC));
    EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), 14);
    const std::int64_t after = ToNs(NowOn(CLOCK_MONOTONIC));
    // Result lies on CLOCK_MONOTONIC, about kAgeNs before the Recv() call.
    EXPECT_GE(ToNs(rx_ts), before - kAgeNs - 1'000'000LL);
    EXPECT_LE(ToNs(rx_ts), after - kAgeNs + 1'000'000LL);
}

TEST(RawSocketTest, Recv_SoftwareTimestamp_ImplausibleAge_FallsBackToReceptionTime)
{
    // A REALTIME step between stamping and reading makes the age negative or huge; the result must stay on
    // CLOCK_MONOTONIC and non-zero instead of carrying the step.
    for (const std::int64_t age_ns : {-3'600'000'000'000LL, 3'600'000'000'000LL})
    {
        FakeOsSyscalls fake;
        fake.poll_result = 1;
        fake.recvmsg_result = 14;
        fake.recvmsg_fill_swts = true;
        fake.recvmsg_swts = RealtimeMinus(age_ns);
        RawSocketImpl sock{&fake};
        OpenSocket(sock, fake);
        std::uint8_t buf[256] = {};
        ::timespec rx_ts{};
        const std::int64_t before = ToNs(NowOn(CLOCK_MONOTONIC));
        EXPECT_EQ(sock.Recv(buf, sizeof(buf), rx_ts, 10), 14);
        const std::int64_t after = ToNs(NowOn(CLOCK_MONOTONIC));
        EXPECT_GE(ToNs(rx_ts), before);
        EXPECT_LE(ToNs(rx_ts), after);
    }
}

// ── RawSocket — Send() ────────────────────────────────────────────────────────

TEST(RawSocketTest, Send_SendFails_ReturnsNegativeOne)
{
    FakeOsSyscalls fake;
    fake.send_result = -1;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    const std::uint8_t data[14] = {};
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(data, 14, tx_ts), -1);
}

TEST(RawSocketTest, Send_Success_PollNoTxTs_ReturnsSentBytes)
{
    FakeOsSyscalls fake;
    fake.send_result = 14;
    fake.poll_result = 0;  // no TX-timestamp event
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    const std::uint8_t data[14] = {};
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(data, 14, tx_ts), 14);
    EXPECT_EQ(tx_ts.tv_sec, 0);
    EXPECT_EQ(tx_ts.tv_nsec, 0);
}

TEST(RawSocketTest, Send_Success_TxTimestampCmsg_ExtractsHwts)
{
    // poll returns POLLERR → MSG_ERRQUEUE recvmsg fills SO_TIMESTAMPING cmsg.
    FakeOsSyscalls fake;
    fake.send_result = 14;
    fake.poll_result = 1;
    fake.poll_revents = POLLERR;
    fake.tx_result = 14;       // TX recvmsg succeeds
    fake.tx_fill_hwts = true;  // inject ts[2]={1,500_000_000}
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    const std::uint8_t data[14] = {};
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(data, 14, tx_ts), 14);
    EXPECT_EQ(tx_ts.tv_sec, 1);
    EXPECT_EQ(tx_ts.tv_nsec, 500'000'000L);
}

TEST(RawSocketTest, Send_Success_OnlySoftwareTxTimestamp_FallsBackToMonotonic)
{
    FakeOsSyscalls fake;
    fake.send_result = 14;
    fake.poll_result = 1;
    fake.poll_revents = POLLERR;
    fake.tx_result = 14;
    fake.tx_fill_swts = true;  // NIC without HW TX timestamping (e.g. virtio)
    fake.tx_swts = RealtimeMinus(0);
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    const std::uint8_t data[14] = {};
    ::timespec tx_ts{};
    const std::int64_t before = ToNs(NowOn(CLOCK_MONOTONIC));
    EXPECT_EQ(sock.Send(data, 14, tx_ts), 14);
    const std::int64_t after = ToNs(NowOn(CLOCK_MONOTONIC));
    EXPECT_NE(ToNs(tx_ts), 0);
    EXPECT_GE(ToNs(tx_ts), before - 1'000'000LL);
    EXPECT_LE(ToNs(tx_ts), after);
}

TEST(RawSocketTest, Send_DrainErrQueue_WhileBodyExecuted)
{
    // recvmsg_drain_limit=1 → first MSG_ERRQUEUE call returns 1 (while body runs),
    // second returns -1 → loop exits. Covers DrainErrQueue's while body.
    FakeOsSyscalls fake;
    fake.send_result = 14;
    fake.poll_result = 0;
    fake.errqueue_drain_count = 1;
    RawSocketImpl sock{&fake};
    OpenSocket(sock, fake);
    const std::uint8_t data[14] = {};
    ::timespec tx_ts{};
    EXPECT_EQ(sock.Send(data, 14, tx_ts), 14);
    // recvmsg was called: 1 DrainErrQueue call (returns 1) + 1 call (returns -1)
    EXPECT_GE(fake.recvmsg_call_count, 2);
}

// ── NetworkIdentity ───────────────────────────────────────────────────────────

TEST(NetworkIdentityTest, GetClockIdentity_BeforeResolve_ReturnsZeroIdentity)
{
    NetworkIdentityImpl ni;
    const ClockIdentity id = ni.GetClockIdentity();
    for (const std::uint8_t b : id.id)
    {
        EXPECT_EQ(b, 0U);
    }
}

TEST(NetworkIdentityTest, Resolve_NonExistentInterface_ReturnsFalse)
{
    NetworkIdentityImpl ni;
    EXPECT_FALSE(ni.Resolve("nonexistent_eth_zzz"));
}

TEST(NetworkIdentityTest, Resolve_NonExistentInterface_GetClockIdentityRemainsZero)
{
    NetworkIdentityImpl ni;
    (void)ni.Resolve("nonexistent_eth_zzz");
    const ClockIdentity id = ni.GetClockIdentity();
    for (const std::uint8_t b : id.id)
    {
        EXPECT_EQ(b, 0U);
    }
}

TEST(NetworkIdentityTest, Resolve_LoInterface_ReturnsTrue)
{
    // lo has MAC 00:00:00:00:00:00; the EUI-48→EUI-64 conversion inserts
    // 0xFF 0xFE at positions 3–4 regardless of the MAC value.
    NetworkIdentityImpl ni;
    EXPECT_TRUE(ni.Resolve("lo"));
}

TEST(NetworkIdentityTest, GetClockIdentity_AfterResolveOnLo_HasFfFeBytes)
{
    NetworkIdentityImpl ni;
    ASSERT_TRUE(ni.Resolve("lo"));
    const ClockIdentity id = ni.GetClockIdentity();
    // EUI-48 → EUI-64: bytes 3 and 4 must be 0xFF and 0xFE
    EXPECT_EQ(id.id[3], 0xFFU);
    EXPECT_EQ(id.id[4], 0xFEU);
}

TEST(NetworkIdentityTest, Resolve_CalledTwice_SecondCallSucceeds)
{
    NetworkIdentityImpl ni;
    ASSERT_TRUE(ni.Resolve("lo"));
    EXPECT_TRUE(ni.Resolve("lo"));
}

}  // namespace details
}  // namespace ts
}  // namespace score
