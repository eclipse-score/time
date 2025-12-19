/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_LIBC_H
#define FLATCFG_UTILS_LIBC_H

#include "flatcfg/fs/_os.h"

#include "score/span.hpp"
#include "score/result/result.h"

#if FLATCFG_INTERNAL_HAS_POSIX
    #include <cstdio>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/statvfs.h>
    #include <unistd.h>
#endif

#if FLATCFG_INTERNAL_HAS_LINUX
    #include <sys/syscall.h>
#endif

#if FLATCFG_INTERNAL_HAS_QNX
    #include <sys/iomsg.h>
#endif

#if FLATCFG_INTERNAL_HAS_MACOS
    #include <libproc.h>
    #include <sys/syscall.h>
#endif

namespace flatcfg
{
namespace utils
{

/// @brief Wrapper around all libc functions which we call.
/// @note  This is not to enable mocking; we can already override all public
///        libc functions because they are weakly linked.
///        This is so that we can change these functions' behaviour without
///        affecting our test framework.
/// @details All wrappers retry infinitely on EINTR. If the libc function
///          sets errno to any other value, a Result with an ErrorCode of
///          kOsError is returned, with support data being set to errno.
///          Errno is NOT cleared by these wrappers.
class Libc
{
public:
#if FLATCFG_INTERNAL_HAS_POSIX
    /// @brief Get the process ID of the calling process.
    /// @headerfile <unistd.h>
    /// @note  Cannot fail.
    static pid_t
    getpid() noexcept;

    /// @brief Obtain file descriptor of a stdio stream.
    /// @headerfile <stdio.h>
    static score::Result<int>
    fileno(std::FILE *stream) noexcept;

    /// @brief Get statistics of the file pointed to by the path.
    /// @warning Path must be null-terminated.
    /// @headerfile <sys/stat.h>
    static score::Result<struct ::stat>
    stat(const char *path) noexcept;

    /// @brief Get statistics of the file pointed to by the path, not following
    ///        symbolic links.
    /// @warning Path must be null-terminated.
    /// @headerfile <sys/stat.h>
    static score::Result<struct ::stat>
    lstat(const char *path) noexcept;

    /// @brief Get statistics of the file pointed to by the file descriptor.
    /// @headerfile <sys/stat.h>
    static score::Result<struct ::stat>
    fstat(int fd) noexcept;

    /// @brief Get statistics of the file named by the name in the directory
    ///        pointed to by the file descriptor.
    /// @warning Path must be null-terminated.
    /// @headerfile <sys/stat.h>
    static score::Result<struct ::stat>
    fstatat(int dirFd, const char *path, int flags) noexcept;

    /// @brief Get statistics of a file relative to a directory file descriptor.
    /// @headerfile <sys/statvfs.h>
    static score::Result<struct ::statvfs>
    fstatvfs(int fd) noexcept;

    /// @brief Read the contents of a symbolic link.
    /// @warning Path must be null-terminated.
    /// @warning Does not append a terminating null byte.
    /// @headerfile <unistd.h>
    static score::Result<std::size_t>
    readlink(const char *path, score::cpp::span<char> buf) noexcept;

    /// @brief Open and possibly create a file.
    /// @warning Path must be null-terminated.
    /// @headerfile <fcntl.h>
    static score::Result<int>
    open(const char *path, int flags) noexcept;

    /// @brief Close a file descriptor.
    /// @headerfile <unistd.h>
    static void
    close(int fd) noexcept;

    /// @brief Read bytes from a file descriptor.
    /// @headerfile <unistd.h>
    /// @note Will loop internally until count bytes are read, eof is reached,
    ///       or an error occurs.
    static score::Result<std::size_t>
    read(int fd, void *buf, std::size_t count) noexcept;

    /// @brief Write bytes to a file descriptor.
    /// @headerfile <unistd.h>
    /// @note Will loop internally until count bytes are written or an error
    ///       occurs.
    /// @warning Special care needs to be taken with logging in write so we
    ///          don't end up with infinite recursion.
    static score::Result<std::size_t>
    write(int fd, const void *buf, std::size_t count) noexcept;
#endif

#if FLATCFG_INTERNAL_HAS_LINUX
    /// @brief Read whole linux_dirent structs into a buffer whose size must be
    ///        at least the filesystem block size.
    /// @headerfile <unistd.h>
    static score::Result<std::size_t>
    sys_getdents64(int fd, void *buf, std::size_t size) noexcept;
#endif

#if FLATCFG_INTERNAL_HAS_QNX
    /// @brief Reads whole dirent structs into a buffer whose size must be at
    ///        least the filesystem block size.
    /// @headerfile <unistd.h>
    static score::Result<std::size_t>
    _readx(int fd, void *buf, std::size_t size,
           unsigned xtype, void *xdata, std::size_t xdataSize) noexcept;
#endif

#if FLATCFG_INTERNAL_HAS_MACOS
    /// @brief Reads whole macos_dirent structs into a buffer whose size must be
    ///        at least the filesystem block size.
    /// @headerfile <unistd.h>
    static score::Result<std::size_t>
    sys_getdirentries64(int fd, void *buf, std::size_t size, long *basep) noexcept;

    /// @brief Reads the path to the executable of the process into a buffer.
    /// @warning The buffer size must be exactly PROC_PIDPATHINFO_MAXSIZE.
    static score::Result<std::size_t>
    proc_pidpath(int pid, void *buf, std::uint32_t size) noexcept;
#endif
};

}  // namespace utils
}  // namespace flatcfg

#endif  // FLATCFG_UTILS_LIBC_H
