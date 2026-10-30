/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_WRAPPER_H
#define SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_WRAPPER_H

#include "score/time/vehicle_time/src/vehicle_time.h"

#include <score/callback.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace score
{
namespace time
{
namespace detail
{

/// @brief Comparison for types that already provide @c operator==.
template <typename Value>
bool IsSame(const Value& first, const Value& second) noexcept
{
    return first == second;
}

/// @brief Status delivery ignores rate deviation. Only the flag set decides whether to notify.
///
/// @c operator== on @c VehicleTimeStatus also compares @c rate_deviation, so delivery must not use it.
inline bool IsSame(const VehicleTimeStatus& first, const VehicleTimeStatus& second) noexcept
{
    return first.flags == second.flags;
}

/// @brief Callback type stored for @c Data; identical to the matching @c VehicleTime callback alias.
template <typename Data>
using SvtCallback = score::cpp::callback<void(const Data&), VehicleTime::kCallbackCapacity>;

/// @brief Thread-safe holder for any number of callbacks that are invoked from a different thread
///        whenever the observed value changes.
///
/// Each callback remembers the last @c Data delivered to it, so @c TryToEnvoke() delivers to it only on
/// change (@c IsSame()). A callback added with @c Add() is delivered by the next @c TryToEnvoke(), whether or
/// not the data changed.
///
/// Guarantees:
///  - @c Add() / @c Unset() may be called from any thread at any time.
///  - @c TryToEnvoke() runs the callbacks while the slot's recursive mutex is held, so:
///     - @c Add() / @c Unset() from another thread block until the in-flight pass has returned.
///       Once @c Unset() returns, none of the removed callbacks is running or will ever be invoked again.
///     - @c Add() / @c Unset() called re-entrantly from inside a callback are safe.
///
/// @tparam Data  Equality-comparable and copyable. Callbacks receive a @c const @c Data&.
template <typename Data>
class SvtCallbackWrapper final
{
  public:
    SvtCallbackWrapper() noexcept = default;
    ~SvtCallbackWrapper() noexcept = default;
    SvtCallbackWrapper(const SvtCallbackWrapper&) = delete;
    SvtCallbackWrapper& operator=(const SvtCallbackWrapper&) = delete;
    SvtCallbackWrapper(SvtCallbackWrapper&&) = delete;
    SvtCallbackWrapper& operator=(SvtCallbackWrapper&&) = delete;

    /// @brief Appends @p callback. An empty callback is ignored.
    void Add(SvtCallback<Data>&& callback)
    {
        if (callback.empty())
        {
            return;
        }
        const std::scoped_lock lock{mutex_};
        entries_.push_back(Entry{std::make_shared<SvtCallback<Data>>(std::move(callback)), std::nullopt});
        is_set_.store(true, std::memory_order_release);
    }

    /// @brief Removes all stored callbacks.
    void Unset() noexcept
    {
        const std::scoped_lock lock{mutex_};
        entries_.clear();
        is_set_.store(false, std::memory_order_release);
    }

    /// @brief Returns @c true if at least one callback is stored.
    bool IsSet() const noexcept
    {
        return is_set_.load(std::memory_order_acquire);
    }

    /// @brief Delivers @p data to each stored callback whose last delivered value differs from it.
    void TryToEnvoke(const Data& data) noexcept
    {
        const std::scoped_lock lock{mutex_};
        // Index-based: a callback may Add() or Unset() re-entrantly, which invalidates references into entries_.
        for (std::size_t index = 0U; index < entries_.size(); ++index)
        {
            if (entries_[index].last_data.has_value() && IsSame(entries_[index].last_data.value(), data))
            {
                continue;
            }
            entries_[index].last_data = data;

            // Local copy keeps the callback alive should it Unset() itself while running.
            const std::shared_ptr<SvtCallback<Data>> callback = entries_[index].callback;
            (*callback)(data);
        }
    }

  private:
    struct Entry
    {
        std::shared_ptr<SvtCallback<Data>> callback;
        std::optional<Data> last_data;
    };

    std::recursive_mutex mutex_;
    std::vector<Entry> entries_{};
    std::atomic_bool is_set_{false};
};

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_WRAPPER_H
