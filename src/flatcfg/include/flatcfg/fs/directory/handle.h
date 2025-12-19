/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_FS_DIRECTORY_HANDLE_H
#define FLATCFG_FS_DIRECTORY_HANDLE_H

#include "entry_view.h"

#include "flatcfg/fs/_os.h"
#include "flatcfg/_export.h"

#include "score/result/result.h"
#include "score/span.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace flatcfg
{
namespace fs
{

/// @brief Thin wrapper around an OS specific directory handle type.
class DirectoryHandle
{
public:
    /// @brief Create a handle from the directory referenced by the path.
    /// @pre   The directory path is null-terminated.
    static score::Result<DirectoryHandle>
    FromPath(std::string_view directoryPath) noexcept;

    /// @brief Check if a path references a directory.
    /// @pre   The path is null-terminated.
    static score::Result<bool>
    isDirectoryPath(std::string_view path) noexcept;

    /// @brief Close the underlying native handle.
    ~DirectoryHandle() noexcept;

    /// @brief Move constructible.
    /// @note  The moved-from handle is closed. Calling a member function on it
    ///        will return an error.
    DirectoryHandle(DirectoryHandle&& other) noexcept;

    /// @brief Move assignable.
    /// @note  The moved-from handle is closed. Calling a member function on it
    ///        will return an error.
    DirectoryHandle&
    operator=(DirectoryHandle&& other) noexcept;

    /// @brief Read one or more directory entries in an OS specific format into
    ///        the buffer provided.
    /// @warning This format might be specific to kernel API functions, and differ
    ///          from common userspace formats (e.g. userspace dirent type).
    /// @pre The buffer size must be greater than or equal to the block size
    ///      associated with the directory.
    /// @note This function will only write complete entries into the buffer.
    /// @returns The number of bytes copied into the buffer. Zero bytes means
    ///          that there are no more entries, equivalent to EOF.
    score::Result<std::size_t>
    readEntriesIntoBuffer(score::cpp::span<char> outputBuffer) const noexcept;

    /// @brief Parse a single entry from a buffer in the format written using
    ///        readEntriesIntoBuffer.
    /// @warning This format might be specific to kernel API functions, and differ
    ///          from common userspace formats (e.g. userspace dirent type).
    /// @pre The buffer points to the start of a complete entry created by
    ///      readEntriesIntoBuffer, with no alignment requirement on the entry.
    score::Result<DirectoryEntryView>
    parseEntryFromBuffer(score::cpp::span<const char> buffer) const noexcept;

private:
    /// @brief Creates a handle for the directory referenced by the path.
    /// @note  If an error occurs, it is written to resOut, and the instance is considered invalid.
    /// @pre   The directory path is null-terminated.
    DirectoryHandle(std::string_view directoryPath, score::ResultBlank *resOut) noexcept;

    /// @brief The underlying native handle.
    std::optional<_os::DirHandleT> m_handleOpt {};

    /// @brief Owned copy of the directory path.
    /// @note  Necessary because QNX doesn't properly implement fstatat.
    std::string m_directoryPath {};
};

}  // namespace fs
}  // namespace flatcfg

#endif  // FLATCFG_FS_DIRECTORY_HANDLE_H
