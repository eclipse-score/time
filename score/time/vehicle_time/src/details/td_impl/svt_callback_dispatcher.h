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
#ifndef SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_DISPATCHER_H
#define SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_DISPATCHER_H

// Internal header — include ONLY from translation units under vehicle_time/src/details/td_impl/.
// NOT part of the public API of td_impl.

#include "score/concurrency/condition_variable.h"
#include "score/time/vehicle_time/src/details/td_impl/svt_callback_wrapper.h"
#include "score/time/vehicle_time/src/vehicle_clock.h"
#include "score/time_daemon/src/ipc/svt/receiver/svt_receiver.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

#include <score/jthread.hpp>
#include <score/stop_token.hpp>

#include <chrono>
#include <memory>
#include <mutex>

namespace score
{
namespace time
{
namespace detail
{

/// @brief Converts PTP status flags from the TimeDaemon IPC representation to
///        the @c ClockStatus<VehicleTime::StatusFlag> representation.
ClockStatus<VehicleTime::StatusFlag> ConvertPtpStatus(const score::td::svt::TimeBaseStatus& ptp_status) noexcept;

/// @brief Owns the worker thread that delivers vehicle-time subscription callbacks.
///
/// The TimeDaemon IPC is a shared-memory segment without a notification facility, so the
/// dispatcher owns a dedicated worker thread that polls the receiver every @p poll_interval.
/// @c Start() enables dispatching once the receiver is initialised. The worker thread is created
/// exactly once, as soon as dispatching is enabled and the first callback has been registered, and
/// runs until the dispatcher is destroyed; while no callback is registered it sleeps until the next
/// registration instead of ticking idly.
/// Every snapshot read from the receiver is offered part by part to the three @c SvtCallbackWrapper s, which
/// dispatch on the worker thread when the part differs from what they last delivered:
///  - @c TimeSlaveSyncData — the sync/follow-up part;
///  - @c PDelayMeasurementData — the pDelay part;
///  - @c VehicleTimeStatus — keyed on the status flags (rate deviation excluded from the comparison).
/// A newly registered callback (first registration, re-registration, or replacement) always receives
/// the first snapshot polled after its registration, and afterwards only changes.
///
/// Set/Unset are safe to call concurrently with an in-flight invocation (see @c SvtCallbackWrapper),
/// including a callback that unsets or replaces itself from the worker thread.
class SvtCallbackDispatcher final
{
  public:
    SvtCallbackDispatcher(std::shared_ptr<score::td::SvtReceiver> receiver,
                          std::chrono::milliseconds poll_interval) noexcept;

    ~SvtCallbackDispatcher() noexcept;
    SvtCallbackDispatcher(const SvtCallbackDispatcher&) = delete;
    SvtCallbackDispatcher& operator=(const SvtCallbackDispatcher&) = delete;
    SvtCallbackDispatcher(SvtCallbackDispatcher&&) = delete;
    SvtCallbackDispatcher& operator=(SvtCallbackDispatcher&&) = delete;

    /// @brief Enables dispatching once the receiver is initialised. Must be called at most once.
    ///
    /// The worker thread is spun up as soon as dispatching is enabled and a callback has been
    /// registered, in whichever order those two happen.
    void Start() noexcept;

    void SetTimeSlaveSyncDataReceivedCallback(VehicleTime::TimeSlaveSyncDataReceivedCallback&& callback) noexcept;

    void UnsetTimeSlaveSyncDataReceivedCallback() noexcept;

    void SetPDelayMeasurementFinishedCallback(VehicleTime::PDelayMeasurementFinishedCallback&& callback) noexcept;

    void UnsetPDelayMeasurementFinishedCallback() noexcept;

    void SetStatusChangedCallback(VehicleTime::StatusChangedCallback&& callback) noexcept;

    void UnsetStatusChangedCallback() noexcept;

  private:
    /// @brief Installs @p callback into @p slot. A non-empty callback also starts or wakes the worker;
    ///        an empty one behaves like the corresponding Unset.
    template <typename Slot, typename Callback>
    void SetCallback(Slot& slot, Callback&& callback) noexcept;

    /// @brief Worker thread body: polls the receiver every @c poll_interval_ while at least one callback is
    ///        registered, otherwise sleeps until the next registration or stop request.
    void WorkerFunction(const score::cpp::stop_token& token) noexcept;

    /// @brief Returns @c true if at least one callback is currently registered.
    bool IsAnyCallbackSet() const noexcept;

    /// @brief Reads one snapshot from the receiver and offers each part to its slot.
    void PollAndDispatch() noexcept;

    /// @brief Records that a non-empty callback was registered: starts the worker if dispatching is enabled,
    ///        or wakes it if it is already running.
    void OnCallbackRegistered() noexcept;

    /// @brief Spawns the worker once dispatching is enabled and a callback has been registered; a no-op
    ///        if it is already running. Must be called with @c worker_mutex_ held.
    void StartWorkerIfReadyLocked() noexcept;

    std::shared_ptr<score::td::SvtReceiver> svt_receiver_;
    const std::chrono::milliseconds poll_interval_;

    SvtCallbackWrapper<VehicleTime::TimeSlaveSyncDataReceivedCallback, score::td::svt::SyncFupSnapshot> sync_data_slot_;
    SvtCallbackWrapper<VehicleTime::PDelayMeasurementFinishedCallback, score::td::svt::PDelayDataSnapshot> pdelay_slot_;
    SvtCallbackWrapper<VehicleTime::StatusChangedCallback, ClockStatus<VehicleTime::StatusFlag>> status_slot_;

    // Guards the worker state below and is the mutex the worker sleeps on. Never held while a slot
    // mutex is taken, so that a callback may (re-)register from the worker thread without risking a
    // lock-order inversion.
    std::mutex worker_mutex_;
    bool enabled_;
    // Set once the first non-empty callback is registered and never cleared: the worker outlives
    // its subscribers, so a later Unset() must not affect it.
    bool callback_registered_;
    // Raised by every registration and consumed by the worker: ends an indefinite sleep, and covers a
    // registration that happens between the worker's slot check and its wait (no lost wake-up).
    bool wakeup_requested_;
    score::concurrency::InterruptibleConditionalVariable worker_wakeup_;
    score::cpp::jthread worker_;
};

}  // namespace detail
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_CALLBACK_DISPATCHER_H
