/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_PROCESS_ITERATOR_H
#define FLATCFG_CONFIG_PROCESS_ITERATOR_H

#include "identifier.h"
#include "lookup_info.h"
#include "range.h"

#include <flatcfg/fs/_os.h>
#include <flatcfg/fs/directory/iterator.h>
#include <flatcfg/_export.h>

#include "score/result/result.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

namespace flatcfg
{
namespace config
{

/// @brief Iterator to iterate through the config files relevant to the current
///        process's executable.
/// @warning The size of this type exceeds a single page of memory.
class ProcessIterator
{
    // friend DaemonIterator so it can construct this type directly for better
    //   initialization of variant
    // RULECHECKER_comment(1, 1, check_friend_declaration, "Necessary for desired functionality.", true)
    friend class DaemonIterator;

public:
    // iterator member typedefs
    using difference_type = std::ptrdiff_t;
    using value_type = score::Result<Identifier>;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::input_iterator_tag;

    /// @brief Create a process iterator which will iterate over relevant
    ///        configuration files in the directory referenced by the path.
    /// @note  The directory path should not have a trailing separator, although
    ///        it is not an error if it does.
    static score::Result<ProcessIterator>
    FromPathAndInfo(std::string_view directoryPath,
                    const LookupInfo& lookupInfo) noexcept;

    /// @brief Default constructor produces a sentinel iterator, equivalent to end().
    ProcessIterator() noexcept;

    /// @brief Move constructible.
    /// @note  The moved-from iterator will become a sentinel iterator.
    // RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "All data members are constructed.", false)
    ProcessIterator(ProcessIterator&&) = default;

    /// @brief Move assignable.
    /// @note  The moved-from iterator will become a sentinel iterator.
    ProcessIterator&
    operator=(ProcessIterator&&) = default;

    /// @brief Progress to the next relevant configuration, or become a sentinel
    ///        iterator if there are no more relevant configurations.
    /// @note  This is valid to call if the iterator is a sentinel, in which
    ///        case it will be unmodified.
    /// @warning It is undefined behaviour to call this if the iterator is in an
    ///          error state.
    ProcessIterator&
    operator++() noexcept;

    /// @brief Progress to the next relevant configuration, or become a sentinel
    ///        iterator if there are no more relevant configurations.
    /// @note  This is valid to call if the iterator is a sentinel, in which
    ///        case it will be unmodified.
    /// @warning It is undefined behaviour to call this if the iterator is in an
    ///          error state.
    void
    operator++(int) noexcept;

    /// @brief Attempt to provide the current relevant configuration.
    /// @note  Any error encountered after construction is surfaced here.
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
    friend bool
    operator==(const ProcessIterator& lhs, const ProcessIterator& rhs) noexcept;

    /// @brief Inequality comparison.
    friend bool
    operator!=(const ProcessIterator& lhs, const ProcessIterator& rhs) noexcept;

    /// @brief Equality comparison with sentinel.
    friend bool
    operator==(const ProcessIterator& lhs, Sentinel rhs) noexcept;

    /// @brief Inequality comparison with sentinel.
    friend bool
    operator!=(const ProcessIterator& lhs, Sentinel rhs) noexcept;

    /// @brief Reversed equality comparison with sentinel.
    friend bool
    operator==(Sentinel lhs, const ProcessIterator& rhs) noexcept;

    /// @brief Reversed inequality comparison with sentinel.
    friend bool
    operator!=(Sentinel lhs, const ProcessIterator& rhs) noexcept;

private:
    /// @brief Create a process iterator which will iterate over relevant
    ///        configuration files in the directory referenced by the path.
    /// @note  The directory path should not have a trailing separator, although
    ///        it is not an error if it does.
    /// @note  If an error occurs, it is written to resOut and the instance is considered invalid.
    ProcessIterator(std::string_view directoryPath, const LookupInfo& lookupInfo,
                    score::ResultBlank *resOut) noexcept;

    /// @brief Get the current error state, either from the underlying iterator
    ///        or from the extra error state.
    score::ResultBlank
    getError() noexcept;

    /// @brief If the underlying iterator is in an error state, copy its error
    ///        and reset it to a sentinel.
    /// @details This should be called at the exit of every non-const public
    ///          member function, to ensure that we do not keep directory
    ///          handles open longer than necessary.
    void
    copyAndClearIteratorError() noexcept;

    /// @brief Advance the iterator to the next entry.
    /// @note  If isError() or isSentinel() is true, this function does nothing.
    /// @returns True if both isError() and isSentinel() would be false after
    ///          calling this function, otherwise false.
    bool
    advanceEntry() noexcept;

    /// @brief Save the name of the current or next relevant entry.
    /// @details Advance the iterator to the next entry while both isError() and
    ///          isSentinel() are false, and the current entry does not refer to
    ///          a relevant configuration file.
    ///          If isError() and isSentinel() are still false, save the name of
    ///          the relevant entry in the path buffer.
    void
    skipIrrelevantEntriesAndSaveName() noexcept;

    /// @brief Directory iterator.
    fs::DirectoryIterator m_dirIt {};

    /// @brief Statically sized buffer to hold the complete path to the current
    ///        directory entry.
    std::array<char, fs::_os::MAX_PATH_LENGTH> m_pathBuffer {};

    /// @brief Size of parent directory path in buffer, including trailing slash.
    std::size_t m_parentDirSize {};

    /// @brief Size of file name in buffer, not including leading slash.
    std::size_t m_nameSize {};

    /// @brief Extra information to check if found files are relevant.
    LookupInfo m_info;

    /// @brief Error state owned independently of underlying iterator.
    /// @note  Prefer getError() over accessing this directly when possible.
    score::ResultBlank m_error {};
};

}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_PROCESS_ITERATOR_H
