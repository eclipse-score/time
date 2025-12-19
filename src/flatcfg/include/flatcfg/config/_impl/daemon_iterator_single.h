/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_SINGLE_H
#define FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_SINGLE_H

#include "daemon_iterator_triple.h"

#include <flatcfg/_export.h>

#include "score/result/result.h"

#include <string_view>

namespace flatcfg
{
namespace config
{
namespace _impl
{

/// @brief Wrapper around triple implementation that combines all underlying
///        iterators so that they can be advanced together.
/// @note  Does not function as an iterator.
class DaemonIteratorSingle
{
public:
    /// @brief Create a daemon iterator single.
    /// @post  Initializes underlying iterators to point to the first entry if
    ///        it exists, a sentinel state if no entries exist, or an error
    ///        state if an error occurs.
    DaemonIteratorSingle() noexcept;

    /// @brief Move constructible.
    /// @note  The moved-from instance will be put in a sentinel state.
    DaemonIteratorSingle(DaemonIteratorSingle&&) = default;

    /// @brief Move assignable.
    /// @note  The moved-from instance will be put in a sentinel state.
    DaemonIteratorSingle&
    operator=(DaemonIteratorSingle&&) = default;

    /// @brief True if in an error state.
    bool
    isError() const noexcept;

    /// @brief True if in a sentinel state.
    bool
    isSentinel() const noexcept;

    /// @brief Is equivalent to !(isError() || isSentinel()).
    // RULECHECKER_comment(1, 1, check_non_explicit_conversion_function, "Non-explicit for easier usage.", true)
    operator bool() const noexcept;

    /// @brief Get a view to the complete path of the current entry.
    /// @pre   operator bool()
    std::string_view
    path() const noexcept;

    /// @brief Get the current error state.
    score::ResultBlank
    getError() const noexcept;

    /// @brief Increment the underlying iterators to the next entry.
    /// @pre   operator bool()
    /// @returns The value of operator bool() after calling this function.
    bool
    advanceEntry() noexcept;

private:
    /// @brief Underlying triple instance.
    DaemonIteratorTriple m_triple {};
};

}  // namespace _impl
}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_IMPL_DAEMON_ITERATOR_SINGLE_H
