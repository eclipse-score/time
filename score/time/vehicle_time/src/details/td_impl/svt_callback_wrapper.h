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

/// @brief Delivery comparison for types that already provide @c operator==.
template <typename Value>
bool IsSameForDelivery(const Value& first, const Value& second) noexcept
{
    return first == second;
}

/// @brief Sync-data delivery compares every field. Port identity is compared member by member.
template <typename Timebase>
bool IsSameForDelivery(const TimeSlaveSyncData<Timebase>& first, const TimeSlaveSyncData<Timebase>& second) noexcept
{
    const bool same_precise_origin_timestamp = (first.precise_origin_timestamp == second.precise_origin_timestamp);
    const bool same_reference_global_timestamp =
        (first.reference_global_timestamp == second.reference_global_timestamp);
    const bool same_reference_local_timestamp = (first.reference_local_timestamp == second.reference_local_timestamp);
    const bool same_sync_ingress_timestamp = (first.sync_ingress_timestamp == second.sync_ingress_timestamp);
    const bool same_correction_field = (first.correction_field == second.correction_field);
    const bool same_sequence_id = (first.sequence_id == second.sequence_id);
    const bool same_pdelay = (first.pdelay == second.pdelay);
    const bool same_clock_identity =
        (first.source_port_identity.clock_identity == second.source_port_identity.clock_identity);
    const bool same_port_number = (first.source_port_identity.port_number == second.source_port_identity.port_number);
    return (same_precise_origin_timestamp && same_reference_global_timestamp && same_reference_local_timestamp &&
            same_sync_ingress_timestamp && same_correction_field && same_sequence_id && same_pdelay &&
            same_clock_identity && same_port_number);
}

/// @brief pDelay delivery compares every field. Port identities are compared member by member.
template <typename Timebase>
bool IsSameForDelivery(const PDelayMeasurementData<Timebase>& first,
                       const PDelayMeasurementData<Timebase>& second) noexcept
{
    const bool same_request_origin_timestamp = (first.request_origin_timestamp == second.request_origin_timestamp);
    const bool same_request_receipt_timestamp = (first.request_receipt_timestamp == second.request_receipt_timestamp);
    const bool same_response_origin_timestamp = (first.response_origin_timestamp == second.response_origin_timestamp);
    const bool same_response_receipt_timestamp =
        (first.response_receipt_timestamp == second.response_receipt_timestamp);
    const bool same_reference_global_timestamp =
        (first.reference_global_timestamp == second.reference_global_timestamp);
    const bool same_reference_local_timestamp = (first.reference_local_timestamp == second.reference_local_timestamp);
    const bool same_sequence_id = (first.sequence_id == second.sequence_id);
    const bool same_pdelay = (first.pdelay == second.pdelay);
    const bool same_request_clock_identity =
        (first.request_port_identity.clock_identity == second.request_port_identity.clock_identity);
    const bool same_request_port_number =
        (first.request_port_identity.port_number == second.request_port_identity.port_number);
    const bool same_response_clock_identity =
        (first.response_port_identity.clock_identity == second.response_port_identity.clock_identity);
    const bool same_response_port_number =
        (first.response_port_identity.port_number == second.response_port_identity.port_number);
    return (same_request_origin_timestamp && same_request_receipt_timestamp && same_response_origin_timestamp &&
            same_response_receipt_timestamp && same_reference_global_timestamp && same_reference_local_timestamp &&
            same_sequence_id && same_pdelay && same_request_clock_identity && same_request_port_number &&
            same_response_clock_identity && same_response_port_number);
}

/// @brief Status delivery ignores rate deviation. Only the flag set decides whether to notify.
inline bool IsSameForDelivery(const VehicleTimeStatus& first, const VehicleTimeStatus& second) noexcept
{
    return first.flags == second.flags;
}

/// @brief Thread-safe holder for a single callback that is invoked from a different thread
///        whenever the observed value changes.
///
/// The slot remembers the last @c Data delivered to the current callback, so
/// @c TryDeliverChangedData() delivers only on change (@c IsSameForDelivery()).  @c Set() forgets that value
/// together with the old callback: the first @c TryDeliverChangedData() after any (re-)registration therefore
/// always delivers.
///
/// Guarantees:
///  - @c Set() / @c Unset() may be called from any thread at any time.
///  - @c TryDeliverChangedData() runs the callback while the slot's recursive mutex is held, so:
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
    /// Forgets the last delivered value, so the next @c TryDeliverChangedData() delivers unconditionally.
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
    /// No-op if no callback is set, or if @c IsSameForDelivery() reports @p data unchanged from the
    /// value previously delivered to the same callback. The callback is invoked with the full @p data.
    ///
    /// @return @c true if the callback was invoked, @c false if none is installed or @p data is unchanged.
    bool TryDeliverChangedData(const Data& data) noexcept
    {
        const std::lock_guard<std::recursive_mutex> lock{mutex_};
        if ((callback_ == nullptr) || (last_data_.has_value() && IsSameForDelivery(last_data_.value(), data)))
        {
            return false;
        }
        last_data_ = data;

        // Local copy keeps the callback alive should it Unset() or replace itself while running.
        const std::shared_ptr<Callback> callback = callback_;
        (*callback)(data);
        return true;
    }

  private:
    std::recursive_mutex mutex_;
    std::shared_ptr<Callback> callback_{};
    std::optional<Data> last_data_{};
    // Mirrors callback_ != nullptr. Written only under mutex_, read lock-free by IsSet().
    std::atomic_bool is_set_{false};
};

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_WRAPPER_H
