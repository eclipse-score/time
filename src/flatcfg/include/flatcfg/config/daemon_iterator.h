/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_DAEMON_ITERATOR_H
#define FLATCFG_CONFIG_DAEMON_ITERATOR_H

#include "_impl/daemon_iterator.h"

#include <flatcfg/config/process_iterator.h>

#include <variant>

namespace flatcfg
{
namespace config
{

/// @brief Iterator to iterate through the config files relevant to a daemon.
/// @warning The size of this type exceeds three pages of memory.
class DaemonIterator
{
public:
    // iterator member typedefs
    using difference_type = std::ptrdiff_t;
    using value_type = score::Result<Identifier>;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::input_iterator_tag;

    /// @brief Create a daemon iterator which will iterate over relevant
    ///        configuration files in implicitly known daemon lookup directories.
    static score::Result<DaemonIterator>
    FromInfo(const LookupInfo& lookupInfo) noexcept;

    /// @brief Create a daemon iterator which will iterate over relevant
    ///        configuration files in the directory referenced by the path.
    static score::Result<DaemonIterator>
    FromPathAndInfo(std::string_view directoryPath,
                    const LookupInfo& lookupInfo) noexcept;

    /// @brief Default constructor produces a sentinel iterator, equivalent to end().
    DaemonIterator() noexcept;

    /// @brief Move constructible.
    /// @note  The moved-from iterator will become a sentinel iterator.
    DaemonIterator(DaemonIterator&&) = default;

    /// @brief Move assignable.
    /// @note  The moved-from iterator will become a sentinel iterator.
    DaemonIterator&
    operator=(DaemonIterator&&) = default;

    /// @brief Progress to the next relevant configuration, or become a sentinel
    ///        iterator if there are no more relevant configurations.
    /// @note  This is valid to call if the iterator is a sentinel, in which
    ///        case it will be unmodified.
    /// @warning It is undefined behaviour to call this if the iterator is in an
    ///          error state.
    DaemonIterator&
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
    operator==(const DaemonIterator& lhs, const DaemonIterator& rhs) noexcept;

    /// @brief Inequality comparison.
    friend bool
    operator!=(const DaemonIterator& lhs, const DaemonIterator& rhs) noexcept;

    /// @brief Equality comparison with sentinel.
    friend bool
    operator==(const DaemonIterator& lhs, Sentinel rhs) noexcept;

    /// @brief Inequality comparison with sentinel.
    friend bool
    operator!=(const DaemonIterator& lhs, Sentinel rhs) noexcept;

    /// @brief Reversed equality comparison with sentinel.
    friend bool
    operator==(Sentinel lhs, const DaemonIterator& rhs) noexcept;

    /// @brief Reversed inequality comparison with sentinel.
    friend bool
    operator!=(Sentinel lhs, const DaemonIterator& rhs) noexcept;

private:
    /// @brief Create a daemon iterator which will iterate over relevant
    ///        configuration files in implicitly known daemon lookup directories.
    /// @note  If an error occurs, it is written to resOut and the instance is considered invalid.
    DaemonIterator(const LookupInfo& lookupInfo, score::ResultBlank *resOut) noexcept;

    /// @brief Create a daemon iterator which will iterate over relevant
    ///        configuration files in the directory referenced by the path.
    /// @note  If an error occurs, it is written to resOut and the instance is considered invalid.
    DaemonIterator(std::string_view directoryPath, const LookupInfo& lookupInfo,
                   score::ResultBlank *resOut) noexcept;

    /// @brief Variable underlying iterator depending on if we're searching
    ///        known daemon directories or falling back to a single directory.
    std::variant<_impl::DaemonIteratorImpl, ProcessIterator> m_itVar {};
};

}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_DAEMON_ITERATOR_H
