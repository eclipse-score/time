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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_OS_SYSCALLS_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_OS_SYSCALLS_H

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstddef>

namespace score
{
namespace ts
{
namespace details
{

/// Thin abstraction over the POSIX system calls used by RawSocket.
/// The default production path calls the real syscalls; a fake can be injected
/// in unit tests to exercise every branch without requiring CAP_NET_RAW.
class OsSyscalls
{
  public:
    virtual ~OsSyscalls() = default;

    virtual int socket_call(int domain, int type, int protocol) noexcept = 0;
    virtual int ioctl_call(int fd, unsigned long req, void* arg) noexcept = 0;
    virtual int bind_call(int fd, const ::sockaddr* addr, ::socklen_t addrlen) noexcept = 0;
    virtual int setsockopt_call(int fd,
                                int level,
                                int optname,
                                const void* optval,
                                ::socklen_t optlen) noexcept = 0;
    virtual int close_call(int fd) noexcept = 0;
    virtual int poll_call(::pollfd* fds, ::nfds_t nfds, int timeout) noexcept = 0;
    virtual ::ssize_t recvmsg_call(int fd, ::msghdr* msg, int flags) noexcept = 0;
    virtual ::ssize_t send_call(int fd, const void* buf, ::size_t len, int flags) noexcept = 0;
};

/// Real production implementation — delegates directly to the OS.
class RealOsSyscalls final : public OsSyscalls
{
  public:
    static RealOsSyscalls& Instance() noexcept
    {
        static RealOsSyscalls s;
        return s;
    }

    int socket_call(int d, int t, int p) noexcept override
    {
        return ::socket(d, t, p);
    }
    int ioctl_call(int fd, unsigned long req, void* arg) noexcept override
    {
        return ::ioctl(fd, req, arg);
    }
    int bind_call(int fd, const ::sockaddr* a, ::socklen_t l) noexcept override
    {
        return ::bind(fd, a, l);
    }
    int setsockopt_call(int fd, int lv, int opt, const void* v, ::socklen_t l) noexcept override
    {
        return ::setsockopt(fd, lv, opt, v, l);
    }
    int close_call(int fd) noexcept override
    {
        return ::close(fd);
    }
    int poll_call(::pollfd* fds, ::nfds_t n, int t) noexcept override
    {
        return ::poll(fds, n, t);
    }
    ::ssize_t recvmsg_call(int fd, ::msghdr* m, int f) noexcept override
    {
        return ::recvmsg(fd, m, f);
    }
    ::ssize_t send_call(int fd, const void* b, ::size_t l, int f) noexcept override
    {
        return ::send(fd, b, l, f);
    }

  private:
    RealOsSyscalls() = default;
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_OS_SYSCALLS_H
