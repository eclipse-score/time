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
#include "score/TimeSlave/code/gptp/details/raw_socket.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/if_ether.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

void DrainErrQueue(int fd) noexcept
{
    char      buf[2048];
    ::iovec   iov{buf, sizeof(buf)};
    char      ctrl[2048];
    ::msghdr  msg{};
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    while (::recvmsg(fd, &msg, MSG_ERRQUEUE) > 0)
    {
    }
}

}  // namespace

RawSocket::~RawSocket()
{
    Close();
}

bool RawSocket::Open(const std::string& iface)
{
    Close();

    const int fd = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_1588));
    if (fd < 0)
        return false;

    ::ifreq ifr{};
    std::strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    if (::ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
    {
        ::close(fd);
        return false;
    }

    ::sockaddr_ll sa{};
    sa.sll_family   = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_1588);
    sa.sll_ifindex  = ifr.ifr_ifindex;
    if (::bind(fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) < 0)
    {
        ::close(fd);
        return false;
    }

    // SO_BINDTODEVICE: best-effort, don't fail if it doesn't work
    (void)::setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
                       iface.c_str(), static_cast<socklen_t>(iface.size()));

    fd_    = fd;
    iface_ = iface;
    return true;
}

bool RawSocket::EnableHwTimestamping()
{
    if (fd_ < 0)
        return false;

    ::ifreq         ifr{};
    ::hwtstamp_config cfg{};
    std::strncpy(ifr.ifr_name, iface_.c_str(), IFNAMSIZ - 1);
    ifr.ifr_data = reinterpret_cast<char*>(&cfg);

    cfg.tx_type   = HWTSTAMP_TX_ON;
    cfg.rx_filter = HWTSTAMP_FILTER_ALL;

    if (::ioctl(fd_, SIOCSHWTSTAMP, &ifr) < 0)
    {
        // Fall back to PTP-only filter
        cfg.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;
        (void)::ioctl(fd_, SIOCSHWTSTAMP, &ifr);
    }

    const int ts_opts = SOF_TIMESTAMPING_TX_HARDWARE |
                        SOF_TIMESTAMPING_RX_HARDWARE |
                        SOF_TIMESTAMPING_RAW_HARDWARE;
    if (::setsockopt(fd_, SOL_SOCKET, SO_TIMESTAMPING,
                     &ts_opts, sizeof(ts_opts)) < 0)
    {
        return false;
    }
    return true;
}

void RawSocket::Close()
{
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
    iface_.clear();
}

int RawSocket::Recv(std::uint8_t* buf, std::size_t buf_len,
                    ::timespec& hwts, int timeout_ms)
{
    if (fd_ < 0 || buf == nullptr || buf_len == 0)
        return -1;

    // Poll with caller-specified timeout
    ::pollfd pfd{fd_, POLLIN, 0};
    const int pr = ::poll(&pfd, 1, timeout_ms);
    if (pr == 0)
        return 0;   // timeout
    if (pr < 0)
        return -1;

    char     ctrl[1024];
    ::iovec  iov{buf, buf_len};
    ::msghdr msg{};
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    const int len = static_cast<int>(::recvmsg(fd_, &msg, 0));
    if (len < 0)
        return -1;

    std::memset(&hwts, 0, sizeof(hwts));
    for (::cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm != nullptr;
         cm             = CMSG_NXTHDR(&msg, cm))
    {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING)
        {
            const auto* ts = reinterpret_cast<const ::timespec*>(CMSG_DATA(cm));
            if (ts[2].tv_sec != 0 || ts[2].tv_nsec != 0)
                hwts = ts[2];
        }
    }
    return len;
}

int RawSocket::Send(const void* buf, int len, ::timespec& hwts)
{
    if (fd_ < 0 || buf == nullptr || len <= 0)
        return -1;

    DrainErrQueue(fd_);

    const int sent = static_cast<int>(::send(fd_, buf, static_cast<std::size_t>(len), 0));
    if (sent < 0)
        return -1;

    // Retrieve TX hardware timestamp from error queue
    ::pollfd pfd{fd_, POLLERR, 0};
    if (::poll(&pfd, 1, -1) > 0 && (pfd.revents & POLLERR) != 0)
    {
        std::uint8_t tmp[2048];
        ::timespec   tx_hwts{};
        (void)Recv(tmp, sizeof(tmp), tx_hwts, 0);
        hwts = tx_hwts;
    }
    else
    {
        std::memset(&hwts, 0, sizeof(hwts));
    }
    return sent;
}

}  // namespace details
}  // namespace ts
}  // namespace score
