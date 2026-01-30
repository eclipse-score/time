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
#include "raw_socket.hpp"
#include "tsync_types.hpp"

#include <cstring>
#include <unistd.h>

#ifdef _QNX710_
#include "qnx_raw_shim.hpp"
#else
#include "linux_raw_shim.hpp"
#endif

namespace tsyncd
{

#ifdef _QNX710_

int setup_raw_socket(int &sockFd, const char *interface_name)
{
    int fd = qnx_raw_open(interface_name);
    if (fd < 0) {
        sockFd = -1;
        return -1;
    }
    sockFd = fd;
    return 0;
}

int raw_recvMsg(int sockFd, void *recv_buffer, ::timespec *hwts, int flag)
{
    if (sockFd < 0 || !recv_buffer || !hwts) {
        return -1;
    }
    const int nonblock = (flag != 0) ? 1 : 0;
    const int max_len = 1500;
    return qnx_raw_recv(sockFd, recv_buffer, max_len, hwts, nonblock);
}

int raw_sendMsg(int sockFd, void *buffer, int bufLen, ::timespec *hwts)
{
    if (sockFd < 0 || !buffer || bufLen <= 0 || !hwts) {
        return -1;
    }
    return qnx_raw_send(sockFd, buffer, bufLen, hwts);
}

#else

int setup_raw_socket(int &sockFd, const char *interface_name)
{
    int fd = linux_raw_open(interface_name);
    if (fd < 0)
        return -1;

    sockFd = fd;
    return 0;
}

int raw_recvMsg(int sockFd, void *recv_buffer, ::timespec *hwts, int flag)
{
    if (sockFd < 0 || !recv_buffer || !hwts)
        return -1;

    char buf[2048];
    const int len = linux_raw_recv(sockFd, buf, static_cast<int>(sizeof(buf)), hwts, flag);
    if (len < 0)
        return -1;

    std::memcpy(recv_buffer, buf, static_cast<size_t>(len));
    return len;
}

int raw_sendMsg(int sockFd, void *buffer, int bufLen, ::timespec *hwts)
{
    return linux_raw_send(sockFd, buffer, bufLen, hwts);
}

#endif

} // namespace tsyncd
