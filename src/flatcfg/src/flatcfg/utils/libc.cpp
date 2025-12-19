/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "libc.h"
#include "ctc.h"
#include "logging.h"
#include "flatcfg/flatcfg_error.h"

#include "score/result/result.h"

#include <cerrno>
#include <iterator>
#include <type_traits>

using score::cpp::span;
using score::Result;

#define ErrnoResV(var) \
    score::MakeUnexpected(static_cast<FlatCfgErrorCode>(errno), "Set errno after returning = " + std::to_string(errno))

#define ErrnoResT(type) \
    score::MakeUnexpected(static_cast<FlatCfgErrorCode>(errno), "Set errno after returning = " + std::to_string(errno))

FLATCFG_SKIPCOV_BEGIN();

namespace flatcfg
{
namespace utils
{

#if FLATCFG_INTERNAL_HAS_POSIX

pid_t
Libc::getpid() noexcept
{
    return ::getpid();
}

Result<int>
Libc::fileno(std::FILE *stream) noexcept
{
    int fd {};
    do {
        fd = ::fileno(stream);
    }
    while (fd < 0 && errno == EINTR);
    if (fd < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno;
        return ErrnoResV(fd);
    }
    return fd;
}

Result<struct ::stat>
Libc::stat(const char *path) noexcept
{
    // RULECHECKER_comment(1, 1, check_union_object, "Union is part of struct ::stat, nothing we can do.", true)
    struct ::stat st {};
    int res {};
    do {
        res = ::stat(path, &st);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " for path: " << path;
        return ErrnoResV(st);
    }
    return st;
}

Result<struct ::stat>
Libc::lstat(const char *path) noexcept
{
    // RULECHECKER_comment(1, 1, check_union_object, "Union is part of struct ::stat, nothing we can do.", true)
    struct ::stat st {};
    int res {};
    do {
        res = ::lstat(path, &st);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " for path: " << path;
        return ErrnoResV(st);
    }
    return st;
}

Result<struct ::stat>
Libc::fstat(int fd) noexcept
{
    // RULECHECKER_comment(1, 1, check_union_object, "Union is part of struct ::stat, nothing we can do.", true)
    struct ::stat st {};
    int res {};
    do {
        res = ::fstat(fd, &st);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " for file descriptor " << fd;
        return ErrnoResV(st);
    }
    return st;
}

Result<struct ::stat>
Libc::fstatat(int dirFd, const char *path, int flags) noexcept
{
    // RULECHECKER_comment(1, 1, check_union_object, "Union is part of struct ::stat, nothing we can do.", true)
    struct ::stat st {};
    int res {};
    do {
        res = ::fstatat(dirFd, path, &st, flags);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " for file descriptor " << dirFd <<
            ", flags " << flags << ", and path: " << path;
        return ErrnoResV(st);
    }
    return st;
}

Result<std::size_t>
Libc::readlink(const char *path, span<char> buf) noexcept
{
    ssize_t ssize {};
    do {
        ssize = ::readlink(path, buf.data(), buf.size());
    }
    while (ssize < 0 && errno == EINTR);
    if (ssize < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " with buffer size " << buf.size() <<
            " for path: " << path;
        return ErrnoResT(std::size_t);
    }
    if (ssize == 0)
    {
        FLATCFG_LOG_WARN() <<
            "read an empty link from path: " << path;
    }
    return static_cast<std::size_t>(ssize);
}

Result<int>
Libc::open(const char *path, int flags) noexcept
{
    int fd {};
    do {
        fd = ::open(path, flags);
    }
    while (fd < 0 && errno == EINTR);
    if (fd < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " for flags " << flags << " and"
            " path: " << path;
        return ErrnoResV(fd);
    }
    return fd;
}

void
Libc::close(int fd) noexcept
{
    while (::close(fd) < 0 && errno == EINTR);
}

Result<std::size_t>
Libc::read(int fd, void *buf, const std::size_t count) noexcept
{
    // skip everything if count == 0 so we don't get stuck in a loop
    if (count == 0U)
    {
        FLATCFG_LOG_DEBUG() <<
            "count is 0; skipping";
        return count;
    }

    // actual implementation
    // RULECHECKER_comment(1, 1, check_reinterpret_cast_extended, "Need to iterate but ::read takes void *.", true)
    auto *uc_buf = reinterpret_cast<unsigned char *>(buf);
    std::size_t bytesRead {};
    ssize_t res {};
    do {
        res = ::read(fd, buf, count - bytesRead);
        if (res > 0)
        {
            std::advance(uc_buf, res);
            bytesRead += static_cast<std::size_t>(res);
        }
    }
    while ((bytesRead != count && res != 0) || (res < 0 && errno == EINTR));
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " with buffer size " << count <<
            " for file descriptor " << fd;
        return ErrnoResV(bytesRead);
    }
    FLATCFG_LOG_VERBOSE() <<
        "read " << bytesRead << " bytes (requested count was " << count <<
        " ) from file descriptor " << fd;
    return bytesRead;
}

Result<std::size_t>
// RULECHECKER_comment(1, 1, check_max_parameters, "Parameters required by ::write.", true)
Libc::write(int fd, const void *buf, std::size_t count) noexcept
{
    // skip everything if count == 0 so we don't get stuck in a loop
    if (count == 0U)
    {
        return 0; // == count
    }

    // actual implementation
    // RULECHECKER_comment(1, 1, check_reinterpret_cast_extended, "Need to iterate but ::write takes void *.", true)
    auto *uc_buf = reinterpret_cast<const unsigned char *>(buf);
    std::size_t bytesWritten {};
    ssize_t res {};
    do {
        res = ::write(fd, buf, count - bytesWritten);
        if (res > 0)
        {
            std::advance(uc_buf, res);
            bytesWritten += static_cast<std::size_t>(res);
        }
    }
    while ((bytesWritten != count) || (res < 0 && errno == EINTR));
    if (res < 0)
    {
        return ErrnoResV(bytesWritten);
    }
    return bytesWritten;
}

#endif

#if FLATCFG_INTERNAL_HAS_LINUX

Result<std::size_t>
Libc::sys_getdents64(int fd, void *buf, std::size_t size) noexcept
{
    long res {};
    do {
        res = ::syscall(SYS_getdents64, fd, buf, size);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " with buffer size " << size <<
            " for file descriptor " << fd;
        return ErrnoResT(std::size_t);
    }
    FLATCFG_LOG_VERBOSE() <<
        "read " << res << " bytes (buffer size was " << size << ") from file"
        " descriptor " << fd;
    return static_cast<std::size_t>(res);
}

#endif

#if FLATCFG_INTERNAL_HAS_QNX

Result<std::size_t>
// RULECHECKER_comment(1, 1, check_max_parameters, "Parameters required by ::_readx.", true)
Libc::_readx(int fd, void *buf, std::size_t size, unsigned xtype, void *xdata,
       std::size_t xdataSize) noexcept
{
    ssize_t res {};
    do {
        res = ::_readx(fd, buf, size, xtype, xdata, xdataSize);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " with buffer size " << size <<
            " for file descriptor " << fd;
        return ErrnoResT(std::size_t);
    }
    FLATCFG_LOG_VERBOSE() <<
        "read " << res << " bytes (buffer size was " << size << ") from file"
        " descriptor " << fd;
    return static_cast<std::size_t>(res);
}

#endif

#if FLATCFG_INTERNAL_HAS_MACOS

Result<std::size_t>
Libc::sys_getdirentries64(int fd, void *buf, std::size_t size, long *basep) noexcept
{
    long res {};
    do {
        res = ::syscall(SYS_getdirentries64, fd, buf, size, basep);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " with buffer size " << size <<
            " for file descriptor " << fd;
        return ErrnoResT(std::size_t);
    }
    FLATCFG_LOG_VERBOSE() <<
        "read " << res << " bytes (buffer size was " << size << ") from file"
        " descriptor " << fd;
    return static_cast<std::size_t>(res);
}

Result<std::size_t>
Libc::proc_pidpath(int pid, void *buf, std::uint32_t size) noexcept
{
    int res {};
    do {
        res = ::proc_pidpath(pid, buf, size);
    }
    while (res < 0 && errno == EINTR);
    if (res < 0)
    {
        FLATCFG_LOG_DEBUG() <<
            "failed with errno " << errno << " with buffer size " << size <<
            " for pid " << pid;
        return ErrnoResT(std::size_t);
    }
    if (res == 0)
    {
        FLATCFG_LOG_WARN() <<
            "read an empty process path for the pid " << pid;
    }
    return static_cast<std::size_t>(res);
}

#endif

}  // namespace utils
}  // namespace flatcfg

FLATCFG_SKIPCOV_END();
