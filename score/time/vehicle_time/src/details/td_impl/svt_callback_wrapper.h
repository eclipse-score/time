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

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

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

/// @brief Thread-safe holder for a single callback that is invoked from a different thread
///        whenever the observed value changes.
///
/// The slot remembers the last @c Data delivered to the current callback, so
/// @c TryToEnvoke() delivers only on change (@c IsSameForDelivery()).  @c Set() forgets that value
/// together with the old callback: the first @c TryToEnvoke() after any (re-)registration therefore
/// always delivers.
///
/// Guarantees:
///  - @c Set() / @c Unset() may be called from any thread at any time.
///  - @c TryToEnvoke() runs the callback while the slot's recursive mutex is held, so:
///     - @c Set() / @c Unset() from another thread block until the in-flight invocation has returned.
///       Once they return, the previously stored callback is neither running nor will it ever be
///       invoked again — the caller may safely destroy whatever the callback referenced.
///     - @c Set() / @c Unset() called re-entrantly from inside the callback take effect immediately;
///       the running invocation completes normally on a shared handle that outlives the slot contents.
///
/// @tparam Callback  A callable wrapper offering @c empty() and @c operator() (e.g. @c score::cpp::callback).
/// @tparam Data      Equality-comparable, copyable value passed to the callback.
template <typename Callback, typename Data>
class SvtCallbackWrapper final
{
  public:
    SvtCallbackWrapper() noexcept = default;
    ~SvtCallbackWrapper() noexcept = default;
    SvtCallbackWrapper(const SvtCallbackWrapper&) = delete;
    SvtCallbackWrapper& operator=(const SvtCallbackWrapper&) = delete;
    SvtCallbackWrapper(SvtCallbackWrapper&&) = delete;
    SvtCallbackWrapper& operator=(SvtCallbackWrapper&&) = delete;

    /// @brief Installs @p callback, replacing any previous one. An empty callback behaves like @c Unset().
    ///
    /// Forgets the last delivered value, so the next @c TryToEnvoke() delivers unconditionally.
    void Set(Callback&& callback) noexcept
    {
        const std::lock_guard<std::recursive_mutex> lock{mutex_};
        if (callback.empty())
        {
            callback_.reset();
        }
        else
        {
            callback_ = std::make_shared<Callback>(std::move(callback));
        }
        last_data_.reset();
        is_set_.store(callback_ != nullptr, std::memory_order_release);
    }

    /// @brief Removes the stored callback.
    void Unset() noexcept
    {
        const std::lock_guard<std::recursive_mutex> lock{mutex_};
        callback_.reset();
        last_data_.reset();
        is_set_.store(false, std::memory_order_release);
    }

    /// @brief Returns @c true if a callback is currently installed.
    bool IsSet() const noexcept
    {
        return is_set_.load(std::memory_order_acquire);
    }

    /// @brief Delivers @p data to the stored callback when it differs from the last delivery.
    ///
    /// No-op if no callback is set, or if @c IsSame() reports @p data unchanged from the
    /// value previously delivered to the same callback. The callback is invoked with the full @p data.
    /// A call while no callback is installed does not update the remembered value.
    void TryToEnvoke(const Data& data) noexcept
    {
        const std::lock_guard<std::recursive_mutex> lock{mutex_};
        if ((callback_ == nullptr) || (last_data_.has_value() && IsSame(last_data_.value(), data)))
        {
            return;
        }
        last_data_ = data;

        // Local copy keeps the callback alive should it Unset() or replace itself while running.
        const std::shared_ptr<Callback> callback = callback_;
        (*callback)(data);
    }

  private:
    std::recursive_mutex mutex_;
    std::shared_ptr<Callback> callback_{};
    std::optional<Data> last_data_{};
    std::atomic_bool is_set_{false};
};

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_WRAPPER_H
