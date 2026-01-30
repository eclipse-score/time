/********************************************************************************************************
# Copyright ©, 2026, ECARX. All rights reserved. This statement is the current and authoritative version.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: chenhao.gao chenhao.gao@ecarxgroup.com
*********************************************************************************************************/
#include "linux_raw_shim.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <linux/if_ether.h>
#include <poll.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netpacket/packet.h>
namespace {

static void clear_msg_errqueue(int sockfd)
{
    char buf[2048];
    struct iovec iov{};
    struct msghdr msg{};
    char control[2048];

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    while (true)
    {
        const ssize_t len = ::recvmsg(sockfd, &msg, MSG_ERRQUEUE);
        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            break;
        }
    }
}

} // namespace

extern "C" {

int linux_raw_open(const char *ifname)
{
    int fd = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_1588));
    if (fd < 0)
    {
        perror("socket(AF_PACKET) failed");
        return -1;
    }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (::ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl(SIOCGIFINDEX) failed");
        ::close(fd);
        return -1;
    }

    struct sockaddr_ll sock_address{};
    sock_address.sll_family = AF_PACKET;
    sock_address.sll_protocol = htons(ETH_P_1588);
    sock_address.sll_ifindex = ifr.ifr_ifindex;
    if (::bind(fd, reinterpret_cast<struct sockaddr *>(&sock_address), sizeof(sock_address)) < 0)
    {
        perror("bind(raw) failed");
        ::close(fd);
        return -1;
    }

    if (::setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, std::strlen(ifname)) != 0)
    {
        perror("setsockopt(SO_BINDTODEVICE) failed");
        // keep running; bind already done
    }

    return fd;
}

int linux_raw_recv(int fd, void *buf, int buf_len, struct timespec *hwts, int flag)
{
    if (fd < 0 || !buf || buf_len <= 0 || !hwts)
        return -1;

    int recvFlag = 0;
    if (flag == 1)
        recvFlag = MSG_ERRQUEUE;

    char control[1024];
    struct iovec iov{};
    struct msghdr recvMsg{};
    std::memset(control, 0, sizeof(control));

    struct pollfd fds[1]{};
    fds[0].fd = fd;
    int poll_res = 1;
    if (flag == 0)
    {
        fds[0].events = POLLIN;
        poll_res = ::poll(fds, 1, -1);
        if (poll_res <= 0)
            return -1;
    }

    iov.iov_base = buf;
    iov.iov_len = static_cast<size_t>(buf_len);
    recvMsg.msg_iov = &iov;
    recvMsg.msg_iovlen = 1;
    recvMsg.msg_control = control;
    recvMsg.msg_controllen = sizeof(control);

    const int len = static_cast<int>(::recvmsg(fd, &recvMsg, recvFlag));
    if (len < 0)
        return -1;

    for (struct cmsghdr *cm = CMSG_FIRSTHDR(&recvMsg); cm != nullptr; cm = CMSG_NXTHDR(&recvMsg, cm))
    {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING)
        {
            auto *ts = reinterpret_cast<struct timespec *>(CMSG_DATA(cm));
            if (ts[2].tv_sec != 0 || ts[2].tv_nsec != 0)
                *hwts = ts[2];
        }
    }

    return len;
}

int linux_raw_send(int fd, void *buf, int len, struct timespec *hwts)
{
    if (fd < 0 || !buf || len <= 0 || !hwts)
        return -1;

    clear_msg_errqueue(fd);

    const int sent = static_cast<int>(::send(fd, buf, len, 0));
    if (sent < 0)
        return -1;

    struct pollfd fds[1]{};
    fds[0].fd = fd;
    fds[0].events = POLLERR;
    const int poll_res = ::poll(fds, 1, -1);
    if (poll_res < 0)
        return -1;

    if (fds[0].revents & POLLERR)
    {
        return linux_raw_recv(fd, buf, len, hwts, 1);
    }
    return sent;
}

} // extern "C"
