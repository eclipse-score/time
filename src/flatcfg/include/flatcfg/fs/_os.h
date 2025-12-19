/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_FS_INTERNAL_OS_H
#define FLATCFG_FS_INTERNAL_OS_H

/// @brief Compiling for a Linux platform.
#if defined(__linux__)
    #define FLATCFG_INTERNAL_HAS_LINUX 1
#else
    #define FLATCFG_INTERNAL_HAS_LINUX 0
#endif

/// @brief Compiling for a QNX platform.
#if defined(__QNXNTO__)
    #define FLATCFG_INTERNAL_HAS_QNX 1
#else
    #define FLATCFG_INTERNAL_HAS_QNX 0
#endif

/// @brief Compiling for a MacOS platform.
#if defined(__MACH__)
    #define FLATCFG_INTERNAL_HAS_MACOS 1
#else
    #define FLATCFG_INTERNAL_HAS_MACOS 0
#endif

// ensure only a single platform is defined
#if (FLATCFG_INTERNAL_HAS_LINUX + \
     FLATCFG_INTERNAL_HAS_MACOS + \
     FLATCFG_INTERNAL_HAS_QNX) > 1
    #error Cannot support multiple platforms at the same time
#endif

/// @brief Compiling for a platform with POSIX support.
#if FLATCFG_INTERNAL_HAS_LINUX || FLATCFG_INTERNAL_HAS_QNX || FLATCFG_INTERNAL_HAS_MACOS
    #define FLATCFG_INTERNAL_HAS_POSIX 1
#else
    #define FLATCFG_INTERNAL_HAS_POSIX 0
#endif

#include <climits>
#include <cstdint>
#include <cstdlib>

#if FLATCFG_INTERNAL_HAS_POSIX
    #include <dirent.h>
#endif

namespace flatcfg
{
namespace fs
{
namespace _os
{

/// @brief Maximum size of an entry name, not including null-terminator.
/// @note OS dependent.
// coverity[autosar_cpp14_a0_1_1_violation] #49860: This constant variable is used to define a maximum length, for this reason it is defined in the global context.
static constexpr unsigned int MAX_NAME_LENGTH =
#if FLATCFG_INTERNAL_HAS_POSIX
    static_cast<unsigned int>(NAME_MAX);
#else
    #error Unsupported OS
#endif

/// @brief Maximum size of a path, not including null-terminator.
/// @note OS dependent.
// coverity[autosar_cpp14_a0_1_1_violation] #49860: This constant variable is used to define a maximum length, for this reason it is defined in the global context.
static constexpr unsigned int MAX_PATH_LENGTH =
#if FLATCFG_INTERNAL_HAS_POSIX
    static_cast<unsigned int>(PATH_MAX);
#else
    #error Unsupported OS
#endif

/// @brief Native handle type to directory, usually corresponding to a file descriptor.
/// @note OS dependent.
using DirHandleT =
#if FLATCFG_INTERNAL_HAS_POSIX
    int;
#else
    #error Unsupported OS
#endif

/// @brief Native handle type to a file, usually corresponding to a file descriptor.
/// @note  OS dependent.
using FileHandleT =
#if FLATCFG_INTERNAL_HAS_POSIX
    int;
#else
    #error Unsupported OS
#endif

/// @brief Native handle type to directory entry.
/// @note  OS dependent.
/// @warning This type may be the type used by an internal kernel level API
///          rather than a public libc API.
using DirEntryKernelT =
#if FLATCFG_INTERNAL_HAS_POSIX
    struct dirent;
#else
    #error Unsupported OS
#endif

/// @brief The minimum size of a buffer which can be passed to the underlying
///        OS API used by readdir.
/// @note OS dependent.
/// @warning This is usually the block size of the filesystem which the directory
///          is stored on, but can be different if checked with the implementation.
// coverity[autosar_cpp14_a0_1_1_violation:FALSE]
static constexpr unsigned int SAFE_READDIR_BUFFER_SIZE =
#if FLATCFG_INTERNAL_HAS_POSIX
    // technically 8kb block sizes exist, but no one uses them
    // qnx 7.1.0's readdir implementation has this value hardcoded
    4096U;
#else
    #error Unsupported OS
#endif

}  // namespace _os
}  // namespace fs
}  // namespace flatcfg

#endif  // FLATCFG_FS_INTERNAL_OS_H
