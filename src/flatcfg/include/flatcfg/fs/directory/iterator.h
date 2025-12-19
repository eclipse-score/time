/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_FS_DIRECTORY_ITERATOR_H
#define FLATCFG_FS_DIRECTORY_ITERATOR_H

#include "entry.h"
#include "entry_view.h"
#include "handle.h"

#include "flatcfg/_export.h"

#include "score/result/result.h"

#include <array>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>

namespace flatcfg
{
namespace fs
{

/// @brief Iterator to iterate through a directory's entries.
/// @warning The size of this type exceeds a single page of memory.
class DirectoryIterator
{
public:
    // iterator member typedefs
    using difference_type = std::ptrdiff_t;
    using value_type = score::Result<DirectoryEntry>;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::input_iterator_tag;

    /// @brief Create a directory iterator pointing to the directory referenced
    ///        by the path.
    /// @pre   The directory path is null-terminated.
    static score::Result<DirectoryIterator>
    FromPath(std::string_view directoryPath) noexcept;

    /// @brief Default constructor produces a sentinel iterator, equivalent to end().
    DirectoryIterator() noexcept;

    /// @brief Move constructible.
    /// @note  The moved-from iterator will become a sentinel iterator.
    DirectoryIterator(DirectoryIterator&& other) noexcept;

    /// @brief Move assignable.
    /// @note  The moved-from iterator will become a sentinel iterator.
    DirectoryIterator&
    operator=(DirectoryIterator&& other) noexcept;

    /// @brief Progress to the next directory entry, or become a sentinel iterator
    ///        if there are no more entries.
    /// @note  This is valid to call if the iterator is a sentinel, in which
    ///        case it will be unmodified.
    /// @warning It is undefined behaviour to call this if the iterator is in an
    ///          error state.
    DirectoryIterator&
    operator++() noexcept;

    /// @brief Progress to the next directory entry, or become a sentinel iterator
    ///        if there are no more entries.
    /// @note  This is valid to call if the iterator is a sentinel, in which
    ///        case it will be unmodified.
    /// @warning It is undefined behaviour to call this if the iterator is in an
    ///          error state.
    void
    operator++(int) noexcept;

    /// @brief Attempt to provide the current directory entry.
    /// @note  Any error encountered after construction is surface here.
    /// @warning It is undefined behaviour to call this on a sentinel iterator.
    value_type
    operator*() noexcept;

    /// @brief True when called on an iterator in an error state.
    bool
    isError() const noexcept;

    /// @brief True when called on a sentinel iterator.
    bool
    isSentinel() const noexcept;

    /// @brief Is equivalent to !(isError() || isSentinel()).
    // RULECHECKER_comment(1, 1, check_non_explicit_conversion_function, "Non-explicit for easier usage.", true)
    operator bool() const noexcept;

    /// @brief Equality comparison.
    // RULECHECKER_comment(1, 1, check_friend_declaration, "Friend is expected for non-member operators.", true)
    friend bool
    operator==(const DirectoryIterator& lhs, const DirectoryIterator& rhs) noexcept;

    /// @brief Inequality comparison.
    friend bool
    operator!=(const DirectoryIterator& lhs, const DirectoryIterator& rhs) noexcept;

private:
    /// @brief Create a directory iterator pointing to the directory referenced by the path.
    /// @note  If an error occurs, it is written to resOut and the instance is considered invalid.
    /// @pre   The directory path is null-terminated.
    DirectoryIterator(std::string_view directoryPath, score::ResultBlank *resOut) noexcept;

    /// @brief Get a view to the entry in the buffer at the current offset.
    /// @note  This will not set m_handleOptRes to an error on failure.
    /// @pre   operator bool() == true
    score::Result<DirectoryEntryView>
    getCurrentEntryView() const noexcept;

    /// @brief Set the buffer position to the next valid directory entry if the
    ///        current directory entry is invalid/deleted (i.e. if d_ino == 0).
    ///        If the current directory entry is valid, the position is not
    ///        advanced.
    /// @note  This will not set m_handleOptRes to an error on failure.
    /// @details If the last directory entry is invalid/deleted, the buffer position
    ///          is set to the size of the buffer.
    /// @pre     operator bool() == true
    score::ResultBlank
    skipInvalidEntries() noexcept;

    /// @brief Statically sized buffer to hold entries read from the directory
    ///        handle.
    std::array<char, _os::SAFE_READDIR_BUFFER_SIZE> m_buffer {};

    /// @brief Offset of data in buffer from the start of the buffer.
    std::size_t m_bufferPos {};

    /// @brief Size of data in buffer from the start of the buffer.
    std::size_t m_bufferSize {};

    /// @brief Directory handle.
    /// @details We have no mechanism to recover from errors, so any error is
    ///          permanent (unless replaced by assignment), and will be propagated
    ///          by any function trying to use the handle.
    ///          If nullopt is contained in the result type, then we are a
    ///          sentinel iterator.
    score::Result<std::optional<DirectoryHandle>> m_handleOptRes {};
};

}  // namespace fs
}  // namespace flatcfg

#endif  // FLATCFG_FS_DIRECTORY_ITERATOR_H
