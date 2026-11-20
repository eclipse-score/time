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
#include "score/time/vehicle_time/src/details/td_impl/svt_callback_dispatcher.h"
#include "score/time/vehicle_time/src/details/td_impl/svt_converters.h"

#include <score/utility.hpp>

#include <string>
#include <tuple>
#include <utility>

namespace score
{
namespace time
{
namespace detail
{

SvtCallbackDispatcher::SvtCallbackDispatcher(std::shared_ptr<score::td::SvtReceiver> receiver,
                                             const std::chrono::milliseconds poll_interval) noexcept
    : svt_receiver_{std::move(receiver)},
      poll_interval_{poll_interval},
      slots_{},
      worker_mutex_{},
      enabled_{false},
      callback_registered_{false},
      wakeup_requested_{false},
      worker_wakeup_{},
      worker_{}
{
}

SvtCallbackDispatcher::~SvtCallbackDispatcher() noexcept
{
    // The destructor never runs on the worker thread, so joining here is always safe.
    if (worker_.joinable())
    {
        score::cpp::ignore = worker_.request_stop();
        worker_.join();
    }
}

void SvtCallbackDispatcher::Start() noexcept
{
    const std::lock_guard<std::mutex> guard{worker_mutex_};
    enabled_ = true;
    StartWorkerIfReadyLocked();
}

void SvtCallbackDispatcher::OnCallbackRegistered() noexcept
{
    const std::lock_guard<std::mutex> guard{worker_mutex_};
    callback_registered_ = true;
    wakeup_requested_ = true;
    StartWorkerIfReadyLocked();
    // Also delivers the first snapshot promptly to a fresh registration instead of after a full poll interval.
    worker_wakeup_.notify_all();
}

void SvtCallbackDispatcher::StartWorkerIfReadyLocked() noexcept
{
    // The worker is never stopped before destruction, so "already running" is simply "already spawned".
    // This is also what makes a callback (re-)registering from the worker thread trivially safe.
    if (!enabled_ || !callback_registered_ || worker_.joinable())
    {
        return;
    }
    worker_ = score::cpp::jthread{score::cpp::jthread::name_hint{std::string{"vt_cb_dispatch"}},
                                  [this](const score::cpp::stop_token token) noexcept {
                                      WorkerFunction(token);
                                  }};
}

void SvtCallbackDispatcher::WorkerFunction(const score::cpp::stop_token& token) noexcept
{
    while (!token.stop_requested())
    {
        WaitForWakeupOrPollInterval(token);

        if (!token.stop_requested() && IsAnyCallbackSet())
        {
            PollAndDispatch();
        }
    }
}

void SvtCallbackDispatcher::WaitForWakeupOrPollInterval(const score::cpp::stop_token& token) noexcept
{
    std::unique_lock<std::mutex> lock{worker_mutex_};
    const auto wake_up = [&wakeup_requested = wakeup_requested_]() noexcept -> bool {
        return wakeup_requested;
    };
    bool wakeup_success = false;
    if (IsAnyCallbackSet())
    {
        wakeup_success = worker_wakeup_.wait_for(lock, token, poll_interval_, wake_up);
    }
    else
    {
        // Nobody to deliver to: sleep until the next registration rather than ticking every interval.
        wakeup_success = worker_wakeup_.wait(lock, token, wake_up);
    }
    if (wakeup_success)
    {
        wakeup_requested_ = false;
    }
}

bool SvtCallbackDispatcher::IsAnyCallbackSet() const noexcept
{
    return std::apply(
        [](const auto&... slot) noexcept {
            return (slot.IsSet() || ...);
        },
        slots_);
}

void SvtCallbackDispatcher::PollAndDispatch() noexcept
{
    const auto snapshot = svt_receiver_->Receive();
    if (!snapshot.has_value())
    {
        return;
    }

    std::get<SyncDataSlot>(slots_).TryToEnvoke(ConvertSyncData(snapshot.value().sync_fup_data));
    std::get<PDelaySlot>(slots_).TryToEnvoke(ConvertPDelayData(snapshot.value().pdelay_data));
    std::get<StatusSlot>(slots_).TryToEnvoke(
        VehicleTimeStatus{ConvertPtpStatus(snapshot.value().status), snapshot.value().rate_deviation});
}

}  // namespace detail
}  // namespace time
}  // namespace score
