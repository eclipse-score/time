/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_FS_DIRECTORY_ENTRY_H
#define FLATCFG_FS_DIRECTORY_ENTRY_H

#include "entry_view.h"

#include "flatcfg/fs/_os.h"
#include "flatcfg/_export.h"

#include "score/result/result.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace flatcfg
{
namespace fs
{

/// @brief Holds all information for a directory entry.
class DirectoryEntry
{
public:
    /// @brief Create an entry from its view counterpart.
    static score::Result<DirectoryEntry>
    FromView(DirectoryEntryView view) noexcept;

    /// @brief Copy (and move) constructible.
    DirectoryEntry(const DirectoryEntry&) = default;

    /// @brief Copy (and move) assignable.
    DirectoryEntry&
    operator=(const DirectoryEntry&) = default;

    /// @brief Name of the entry.
    /// @note  Will always be non-empty and null-terminated.
    std::string_view
    name() const noexcept;

    /// @brief Type of the entry.
    FileType
    type() const noexcept;

private:
    /// @brief Create an entry from its view counterpart.
    /// @note  If an error occurs, it is written to resOut and the instance is considered invalid.
    DirectoryEntry(DirectoryEntryView view, score::ResultBlank *resOut) noexcept;

    /// @brief Statically sized buffer to hold the entry name.
    std::array<char, _os::MAX_NAME_LENGTH + 1U> m_nameBuffer {};

    /// @brief Size of entry name, not including null-terminator.
    std::size_t m_nameSize {};

    /// @brief Type of the entry.
    FileType m_type {};
};

}  // namespace fs
}  // namespace flatcfg

#endif  // FLATCFG_FS_DIRECTORY_ENTRY_H
