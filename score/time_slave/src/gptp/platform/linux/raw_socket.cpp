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
#include "score/time_slave/src/gptp/details/ptp_types.h"
#include "score/time_slave/src/gptp/details/raw_socket_impl.h"

#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <mutex>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

void DrainErrQueue(int fd, OsSyscalls& sys) noexcept
{
    char buf[2048];
    ::iovec iov{buf, sizeof(buf)};
    char ctrl[2048];
    ::msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    while (sys.recvmsg_call(fd, &msg, MSG_ERRQUEUE) > 0)
    {
    }
}

}  // namespace

RawSocketImpl::RawSocketImpl(OsSyscalls* sys) noexcept : sys_{sys != nullptr ? sys : &RealOsSyscalls::Instance()} {}

RawSocketImpl::~RawSocketImpl()
{
    Close();
}

bool RawSocketImpl::Open(const std::string& iface)
{
    Close();

    const int fd = sys_->socket_call(AF_PACKET, SOCK_RAW, htons(ETH_P_1588));
    if (fd < 0)
        return false;

    ::ifreq ifr{};
    std::strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (sys_->ioctl_call(fd, SIOCGIFINDEX, &ifr) < 0)
    {
        sys_->close_call(fd);
        return false;
    }

    ::sockaddr_ll sa{};
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_1588);
    sa.sll_ifindex = ifr.ifr_ifindex;
    if (sys_->bind_call(fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) < 0)
    {
        sys_->close_call(fd);
        return false;
    }

    // SO_BINDTODEVICE: best-effort, don't fail if it doesn't work
    (void)sys_->setsockopt_call(fd, SOL_SOCKET, SO_BINDTODEVICE, iface.c_str(), static_cast<socklen_t>(iface.size()));

    fd_.store(fd, std::memory_order_release);
    iface_ = iface;
    return true;
}

namespace
{

/// Largest packet age (REALTIME now - kernel timestamp) still considered plausible for a timestamp that is
/// converted right after reception or transmission. Anything outside [0, kMaxPlausibleAgeNs] means that
/// CLOCK_REALTIME was stepped in between (or the packet sat unread for an unusually long time).
constexpr std::int64_t kMaxPlausibleAgeNs = 1'000'000'000LL;

/// The kernel's software timestamp is CLOCK_REALTIME, which can step. Jump detection and the peer delay
/// computation compare local timestamps with each other, so a local step must not show up in them: re-express the
/// timestamp on CLOCK_MONOTONIC by subtracting the packet's age (REALTIME now - timestamp) from MONOTONIC now.
/// If the age is implausible (a REALTIME step happened in between), the packet is assumed to have just arrived
/// (age 0), which is off only by the short time the packet waited in the socket queue.
///
/// @return A non-zero CLOCK_MONOTONIC timestamp, or a zeroed timespec ("no timestamp") if a clock cannot be read.
::timespec SoftwareToMonotonic(const ::timespec& real_ts) noexcept
{
    ::timespec real_now{};
    ::timespec mono_now{};
    if (::clock_gettime(CLOCK_REALTIME, &real_now) != 0 || ::clock_gettime(CLOCK_MONOTONIC, &mono_now) != 0)
        return ::timespec{};

    std::int64_t age_ns = (static_cast<std::int64_t>(real_now.tv_sec) - real_ts.tv_sec) * kNsPerSec +
                          (static_cast<std::int64_t>(real_now.tv_nsec) - real_ts.tv_nsec);
    if (age_ns < 0 || age_ns > kMaxPlausibleAgeNs)
        age_ns = 0;

    std::int64_t mono_ns = static_cast<std::int64_t>(mono_now.tv_sec) * kNsPerSec + mono_now.tv_nsec - age_ns;
    if (mono_ns <= 0)
        mono_ns = 1;  // an all-zero timespec means "no timestamp" to the caller
    return ::timespec{static_cast<::time_t>(mono_ns / kNsPerSec), static_cast<long>(mono_ns % kNsPerSec)};
}

}  // namespace

bool RawSocketImpl::EnableTimestamping()
{
    const int fd = fd_.load(std::memory_order_relaxed);
    if (fd < 0)
        return false;

    ::ifreq ifr{};
    ::hwtstamp_config cfg{};
    std::strncpy(ifr.ifr_name, iface_.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    ifr.ifr_data = reinterpret_cast<char*>(&cfg);

    cfg.tx_type = HWTSTAMP_TX_ON;
    cfg.rx_filter = HWTSTAMP_FILTER_ALL;

    if (sys_->ioctl_call(fd, SIOCSHWTSTAMP, &ifr) < 0)
    {
        // Fall back to PTP-only filter
        cfg.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;
        (void)sys_->ioctl_call(fd, SIOCSHWTSTAMP, &ifr);
    }

    // Hardware timestamps are preferred. The software RX/TX timestamps are only used by Recv()/Send() when the NIC
    // delivers no hardware timestamp (e.g. virtio).
    const int ts_opts = SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE |
                        SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
    if (sys_->setsockopt_call(fd, SOL_SOCKET, SO_TIMESTAMPING, &ts_opts, sizeof(ts_opts)) < 0)
    {
        return false;
    }
    return true;
}

void RawSocketImpl::Close()
{
    const int fd = fd_.exchange(-1, std::memory_order_acq_rel);
    if (fd >= 0)
        sys_->close_call(fd);
    iface_.clear();
}

int RawSocketImpl::Recv(std::uint8_t* buf, std::size_t buf_len, ::timespec& rx_ts, int timeout_ms)
{
    const int fd = fd_.load(std::memory_order_acquire);
    if (fd < 0 || buf == nullptr || buf_len == 0)
        return -1;

    // Poll with caller-specified timeout
    ::pollfd pfd{fd, POLLIN, 0};
    const int pr = sys_->poll_call(&pfd, 1, timeout_ms);
    if (pr == 0)
        return 0;  // timeout
    if (pr < 0)
        return -1;

    char ctrl[1024];
    ::iovec iov{buf, buf_len};
    ::msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    const int len = static_cast<int>(sys_->recvmsg_call(fd, &msg, 0));
    if (len < 0)
        return -1;

    std::memset(&rx_ts, 0, sizeof(rx_ts));
    for (::cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm != nullptr; cm = CMSG_NXTHDR(&msg, cm))
    {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING)
        {
            if (cm->cmsg_len < CMSG_LEN(3 * sizeof(::timespec)))
                continue;
            const auto* ts = reinterpret_cast<const ::timespec*>(CMSG_DATA(cm));
            if (HasTimestamp(ts[2]))
                rx_ts = ts[2];
            else if (HasTimestamp(ts[0]))
                rx_ts = SoftwareToMonotonic(ts[0]);
        }
    }
    return len;
}

int RawSocketImpl::Send(const void* buf, int len, ::timespec& tx_ts)
{
    const int fd = fd_.load(std::memory_order_acquire);
    if (fd < 0 || buf == nullptr || len <= 0)
        return -1;

    std::lock_guard<std::mutex> lk(send_mutex_);
    DrainErrQueue(fd, *sys_);

    const int sent = static_cast<int>(sys_->send_call(fd, buf, static_cast<std::size_t>(len), 0));
    if (sent < 0)
        return -1;

    // TX timestamps arrive via the error queue: software first, then hardware (if the NIC supports it) as a separate
    // message. Once a software one is in, only wait briefly for the hardware one, since e.g. virtio never sends it.
    constexpr int kTxTsTimeoutMs = 50;
    constexpr int kTxTsHwAfterSwTimeoutMs = 2;
    constexpr int kMaxTxTsMessages = 4;
    std::memset(&tx_ts, 0, sizeof(tx_ts));
    ::timespec hw_ts{};
    ::timespec sw_ts{};
    for (int i = 0; i < kMaxTxTsMessages && !HasTimestamp(hw_ts); ++i)
    {
        ::pollfd pfd{fd, POLLERR, 0};
        const int timeout_ms = HasTimestamp(sw_ts) ? kTxTsHwAfterSwTimeoutMs : kTxTsTimeoutMs;
        if (sys_->poll_call(&pfd, 1, timeout_ms) <= 0 || (pfd.revents & POLLERR) == 0)
            break;

        std::uint8_t tmp[2048];
        ::iovec iov{tmp, sizeof(tmp)};
        char ctrl[512];
        ::msghdr msg{};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = ctrl;
        msg.msg_controllen = sizeof(ctrl);

        if (sys_->recvmsg_call(fd, &msg, MSG_ERRQUEUE) < 0)
            break;
        for (::cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm != nullptr; cm = CMSG_NXTHDR(&msg, cm))
        {
            if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING)
            {
                // SO_TIMESTAMPING delivers three timespec values: [0]=SW, [1]=HW-transformed,
                // [2]=HW-raw.  Verify the cmsg payload is large enough before indexing.
                if (cm->cmsg_len < CMSG_LEN(3 * sizeof(::timespec)))
                    continue;
                const auto* ts = reinterpret_cast<const ::timespec*>(CMSG_DATA(cm));
                if (HasTimestamp(ts[2]))
                    hw_ts = ts[2];
                else if (HasTimestamp(ts[0]))
                    sw_ts = ts[0];
            }
        }
    }
    // No hardware TX timestamp: fall back to the software one, expressed on CLOCK_MONOTONIC like the RX fallback.
    if (HasTimestamp(hw_ts))
        tx_ts = hw_ts;
    else if (HasTimestamp(sw_ts))
        tx_ts = SoftwareToMonotonic(sw_ts);
    return sent;
}

}  // namespace details
}  // namespace ts
}  // namespace score
