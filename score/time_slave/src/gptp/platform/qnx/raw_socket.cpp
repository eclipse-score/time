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
#include "score/time_slave/src/gptp/details/raw_socket_impl.h"

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// QNX raw shim C linkage (provided by existing qnx_raw_shim target)
extern "C" {
int qnx_raw_open(const char* ifname);
int qnx_raw_recv(int fd, void* buf, int len, ::timespec* hwts, int nonblock);
int qnx_raw_send(int fd, void* buf, int len, ::timespec* hwts);
}  // extern "C"

namespace score
{
namespace ts
{
namespace details
{

RawSocketImpl::RawSocketImpl(OsSyscalls* /*sys*/) noexcept {}

RawSocketImpl::~RawSocketImpl()
{
    Close();
}

bool RawSocketImpl::Open(const std::string& iface)
{
    Close();
    fd_ = qnx_raw_open(iface.c_str());
    if (fd_ < 0)
        return false;
    iface_ = iface;
    return true;
}

bool RawSocketImpl::EnableHwTimestamping()
{
    // HW timestamping configured inside qnx_raw_open; nothing more needed.
    return true;
}

void RawSocketImpl::Close()
{
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
    iface_.clear();
}

int RawSocketImpl::Recv(std::uint8_t* buf, std::size_t buf_len, ::timespec& hwts, int timeout_ms)
{
    if (fd_ < 0 || buf == nullptr || buf_len == 0)
        return -1;

    const int nonblock = (timeout_ms == 0) ? 1 : 0;
    // QNX shim: nonblock==0 means blocking; only full non-blocking is supported.
    // For timeout > 0 we fall back to a blocking call (best effort).
    (void)timeout_ms;
    return qnx_raw_recv(fd_, buf, static_cast<int>(buf_len), &hwts, nonblock);
}

int RawSocketImpl::Send(const void* buf, int len, ::timespec& hwts)
{
    if (fd_ < 0 || buf == nullptr || len <= 0)
        return -1;
    return qnx_raw_send(fd_, const_cast<void*>(buf), len, &hwts);
}

}  // namespace details
}  // namespace ts
}  // namespace score
