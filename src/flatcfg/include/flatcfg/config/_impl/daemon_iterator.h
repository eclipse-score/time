/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_H
#define FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_H

#include "daemon_iterator_single.h"

#include <flatcfg/config/identifier.h>
#include <flatcfg/config/lookup_info.h>
#include <flatcfg/config/range.h>
#include <flatcfg/_export.h>

#include "score/result/result.h"

#include <cstddef>
#include <iterator>

namespace flatcfg
{
namespace config
{

/// @cond Forward declaration so that friend can find the right type.
class DaemonIterator;
/// @endcond

namespace _impl
{

/// @brief Iterator to iterate through the config files relevant to a daemon.
/// @note  This type does not have the single directory fallback.
class DaemonIteratorImpl
{
    // friend DaemonIterator so it can construct this type directly for better
    //   initialization of variant
    // RULECHECKER_comment(1, 1, check_friend_declaration, "Necessary for desired functionality.", true)
    friend class config::DaemonIterator;

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
    static score::Result<DaemonIteratorImpl>
    FromInfo(const LookupInfo& lookupInfo) noexcept;

    /// @brief Default constructor produces a sentinel iterator, equivalent to end().
    DaemonIteratorImpl() noexcept;

    /// @brief Move constructible.
    /// @note  The moved-from iterator will become a sentinel iterator.
    // RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "All data members are constructed.", false)
    DaemonIteratorImpl(DaemonIteratorImpl&&) = default;

    /// @brief Move assignable.
    /// @note  The moved-from iterator will become a sentinel iterator.
    DaemonIteratorImpl&
    operator=(DaemonIteratorImpl&&) = default;

    /// @brief Progress to the next relevant configuration, or become a sentinel
    ///        iterator if there are no more relevant configurations.
    /// @note  This is valid to call if the iterator is a sentinel, in which
    ///        case it will be unmodified.
    /// @warning It is undefined behaviour to call this if the iterator is in an
    ///          error state.
    DaemonIteratorImpl&
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
    operator*() const noexcept;

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
    operator==(const DaemonIteratorImpl& lhs, const DaemonIteratorImpl& rhs) noexcept;

    /// @brief Inequality comparison.
    friend bool
    operator!=(const DaemonIteratorImpl& lhs, const DaemonIteratorImpl& rhs) noexcept;

    /// @brief Equality comparison with sentinel.
    friend bool
    operator==(const DaemonIteratorImpl& lhs, Sentinel rhs) noexcept;

    /// @brief Inequality comparison with sentinel.
    friend bool
    operator!=(const DaemonIteratorImpl& lhs, Sentinel rhs) noexcept;

    /// @brief Reversed equality comparison with sentinel.
    friend bool
    operator==(Sentinel lhs, const DaemonIteratorImpl& rhs) noexcept;

    /// @brief Reversed inequality comparison with sentinel.
    friend bool
    operator!=(Sentinel lhs, const DaemonIteratorImpl& rhs) noexcept;

private:
    /// @brief Create a daemon iterator which will iterate over relevant
    ///        configuration files in implicitly known daemon lookup directories.
    /// @note  If an error occurs, it is written to resOut and the instance is considered invalid.
    DaemonIteratorImpl(const LookupInfo& lookupInfo, score::ResultBlank *resOut) noexcept;

    /// @brief Underlying core implementation.
    DaemonIteratorSingle m_single {};

    /// @brief Extra information to check if found files are relevant.
    LookupInfo m_info;
};

}  // namespace _impl
}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_H
