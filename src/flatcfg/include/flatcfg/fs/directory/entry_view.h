/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_FS_DIRECTORY_ENTRY_VIEW_H
#define FLATCFG_FS_DIRECTORY_ENTRY_VIEW_H

#include "flatcfg/fs/file/type.h"
#include "flatcfg/fs/_os.h"
#include "flatcfg/_export.h"

#include "score/result/result.h"
#include "score/span.hpp"

#include <cstdint>
#include <string_view>

namespace flatcfg
{
namespace fs
{

/// @brief Cheap directory entry type where the name field is a view rather than owned.
class DirectoryEntryView
{
public:
    /// @brief Typedef because name on some platforms uses wchar_t.
#if FLATCFG_INTERNAL_HAS_POSIX
    using ViewT = std::string_view;
#else
    #error Unsupported OS
#endif

    /// @brief Create an instance of DirectoryEntryView.
    /// @pre   The name is non-empty and does not contain null bytes.
    static score::Result<DirectoryEntryView>
    FromValues(std::uint64_t id, std::uint64_t entrySize, FileType type,
               ViewT name) noexcept;

    /// @brief Copy (and move) constructible.
    DirectoryEntryView(const DirectoryEntryView&) = default;

    /// @brief Copy (and move) assignable.
    DirectoryEntryView&
    operator=(const DirectoryEntryView&) = default;

    /// @brief Get the file identifier (usually corresponding to inode).
    std::uint64_t
    id() const noexcept;

    /// @brief Get the size of the complete entry in bytes.
    std::uint64_t
    entrySize() const noexcept;

    /// @brief Get the type of the file.
    FileType
    type() const noexcept;

    /// @brief Get non-empty view of the file name which does not contain null
    ///        bytes in the platform native format.
    ViewT
    name() const noexcept;

private:
    /// @brief Create an instance of DirectoryEntryView where the name is a view
    ///        into the provided buffer.
    /// @pre   The name is non-empty and does not contain null bytes.
    /// @note  If an error occurs, it is written to resOut, and the instance is considered invalid.
    DirectoryEntryView(std::uint64_t id, std::uint64_t entrySize, FileType type,
                       ViewT name, score::ResultBlank *resOut) noexcept;

    /// @brief File identifier (usually corresponding to inode).
    std::uint64_t m_id {};

    /// @brief Size of the complete entry in bytes.
    std::uint64_t m_entrySize {};

    /// @brief Type of the file.
    FileType m_type {};

    /// @brief Non-empty view of the file name which does not contain null bytes
    ///        in the platform native format.
    ViewT m_name {};
};

}  // namespace fs
}  // namespace flatcfg

#endif  // FLATCFG_FS_DIRECTORY_ENTRY_VIEW_H
