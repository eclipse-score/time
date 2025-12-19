/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_TRIPLE_H
#define FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_TRIPLE_H

#include <flatcfg/fs/_os.h>
#include <flatcfg/fs/directory/iterator.h>
#include <flatcfg/_export.h>

#include "score/result/result.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace flatcfg
{
namespace config
{
namespace _impl
{

/// @brief Base implementation of a daemon iterator where each underlying
///        iterator implicitly knows its lookup directories but must be advanced
///        manually.
class DaemonIteratorTriple
{
public:
    /// @brief Create a daemon iterator triple.
    /// @post  All underlying iterators are in a sentinel state.
    DaemonIteratorTriple() = default;

    /// @brief Move constructible.
    /// @note  The moved-from instance will be put in a sentinel state.
    DaemonIteratorTriple(DaemonIteratorTriple&&) = default;

    /// @brief Move assignable.
    /// @note  The moved-from instance will be put in a sentinel state.
    DaemonIteratorTriple&
    operator=(DaemonIteratorTriple&&) = default;

    /// @brief True if in an error state.
    bool
    isError() const noexcept;

    /// @brief True if not in an error state and all underlying iterators are
    ///        sentinels.
    bool
    isSentinel() const noexcept;

    /// @brief Is equivalent to !(isError() || isSentinel()).
    // RULECHECKER_comment(1, 1, check_non_explicit_conversion_function, "Non-explicit for easier usage.", true)
    operator bool() const noexcept;

    /// @brief Get a view to the complete path of the current entry.
    /// @pre   operator bool() == true
    std::string_view
    path() const noexcept;

    /// @brief Get the current error state.
    score::ResultBlank
    getError() const noexcept;

    /// @brief Put the instance into an error state with the given error.
    /// @pre   !error.HasValue()
    template <class T>
    void
    setError(const score::Result<T>& error) noexcept
    {
        _setVoidError(score::MakeUnexpected<score::Blank>(error.error()));
    }

    /// @brief Advance the root iterator, constructed from /opt/score, to the
    ///        next directory entry.
    /// @note  If the root iterator is a sentinel when called, it will iterate
    ///        from the start of /opt/score, possibly removing the sentinel
    ///        state.
    /// @pre   !isError() && mount.isSentinel()
    /// @post  If true is returned, the root iterator will point to an entry
    ///        which represents a directory.
    /// @returns The value of root.operator bool() after calling this function.
    /// @note The path written to the buffer will match:
    ///          "/opt/score/<mount>/"
    bool
    advanceRootEntry() noexcept;

    /// @brief Advance the mount iterator, constructed from "/opt/score/<mount>",
    ///        to the next directory entry.
    /// @note  If the mount iterator is a sentinel when called, it will iterate
    ///        from the start of the current root iterator entry, possibly
    ///        removing the sentinel state.
    /// @note  The next directory entry's name will have /etc appended to it,
    ///        and will be checked for validity.
    /// @pre   !isError() && root.operator bool() && swcl.isSentinel()
    /// @post  If true is returned, the mount iterator will point to an entry
    ///        which represents a directory.
    /// @returns The value of mount.operator bool() after calling this function.
    /// @note   The path written to the buffer will match:
    ///         "/opt/score/<mount>/<swcl>/etc/"
    bool
    advanceMountEntry() noexcept;

    /// @brief Advance the swcl iterator, constructed from
    ///        "/opt/score/<mount>/<swcl>/etc", to the next entry.
    /// @note  If the swcl iterator is a sentinel when called, it will iterate
    ///        from the start of the current mount iterator entry, possibly
    ///        removing the sentinel state.
    /// @pre   !isError() && root.operator bool() && mount.operator bool()
    /// @post  If true is returned, the swcl iterator will point to an entry
    ///        which represents a regular file.
    /// @post  The path written to the buffer is not null-terminated, or
    ///        terminated with anything else.
    /// @returns The value of swcl.operator bool() after calling this function.
    /// @note The path written to the buffer will match:
    ///          "/opt/score/<mount>/<swcl>/etc/<entry>"
    bool
    advanceSwclEntry() noexcept;

private:
    /// @brief Non-generic implementation of setError.
    /// @pre   !error.HasValue()
    void
    _setVoidError(score::ResultBlank error) noexcept;

    /// @brief Helper type combining a directory iterator and metadata.
    // RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "All data members are constructed.", false)
    struct DirWithInfo
    {
        /// @brief Directory iterator.
        fs::DirectoryIterator it;

        /// @brief Size of the complete parent directory path in buffer
        ///        corresponding to iterator, including trailing separator.
        std::size_t parentSize {};

        /// @brief Size of file name in buffer corresponding to iterator position.
        std::size_t nameSize {};
    };

    /// @brief Array of our directory iterators and their metadata.
    std::array<DirWithInfo, 3> m_dirs {};

    /// @brief Statically sized buffer to hold the complete path to the current
    ///        directory entry.
    std::array<char, fs::_os::MAX_PATH_LENGTH> m_pathBuffer {};

    /// @brief Error state owned independently of underlying iterators.
    score::ResultBlank m_error {};
};

}  // namespace _impl
}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_TRIPLE_H
