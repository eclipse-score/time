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
#ifndef SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_TEST_HELPERS_H
#define SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_TEST_HELPERS_H

// Test-only helpers shared by the SvtCallbackDispatcher and VehicleClockBackendImpl unit tests.

#include "score/time_daemon/src/ipc/svt/receiver/svt_receiver.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>

namespace score
{
namespace time
{
namespace test_helpers
{

constexpr std::chrono::milliseconds kPollInterval{1};
constexpr std::chrono::seconds kWaitTimeout{5};

constexpr score::td::svt::TimeBaseStatus MakeSvtStatus(const bool synchronized,
                                                       const bool timeout,
                                                       const bool leap_future,
                                                       const bool leap_past,
                                                       const bool correct) noexcept
{
    score::td::svt::TimeBaseStatus status{};
    status.is_synchronized = synchronized;
    status.is_timeout = timeout;
    status.is_time_jump_future = leap_future;
    status.is_time_jump_past = leap_past;
    status.is_correct = correct;
    return status;
}

constexpr score::td::svt::TimeBaseStatus kSynchronizedStatus = MakeSvtStatus(/*synchronized=*/true,
                                                                             /*timeout=*/false,
                                                                             /*leap_future=*/false,
                                                                             /*leap_past=*/false,
                                                                             /*correct=*/true);
constexpr score::td::svt::TimeBaseStatus kTimeoutStatus = MakeSvtStatus(/*synchronized=*/true,
                                                                        /*timeout=*/true,
                                                                        /*leap_future=*/false,
                                                                        /*leap_past=*/false,
                                                                        /*correct=*/true);
constexpr score::td::svt::TimeBaseStatus kTimeLeapFutureStatus = MakeSvtStatus(/*synchronized=*/false,
                                                                               /*timeout=*/false,
                                                                               /*leap_future=*/true,
                                                                               /*leap_past=*/false,
                                                                               /*correct=*/true);
constexpr score::td::svt::TimeBaseStatus kTimeLeapPastStatus = MakeSvtStatus(/*synchronized=*/false,
                                                                             /*timeout=*/false,
                                                                             /*leap_future=*/false,
                                                                             /*leap_past=*/true,
                                                                             /*correct=*/true);
constexpr score::td::svt::TimeBaseStatus kNoFlagsStatus = MakeSvtStatus(/*synchronized=*/false,
                                                                        /*timeout=*/false,
                                                                        /*leap_future=*/false,
                                                                        /*leap_past=*/false,
                                                                        /*correct=*/true);
constexpr score::td::svt::TimeBaseStatus kNotCorrectStatus = MakeSvtStatus(/*synchronized=*/true,
                                                                           /*timeout=*/false,
                                                                           /*leap_future=*/false,
                                                                           /*leap_past=*/false,
                                                                           /*correct=*/false);

inline score::td::svt::TimeBaseSnapshot MakeSvtSnapshot(const score::td::svt::TimeBaseStatus status,
                                                        const double rate_deviation = 0.0) noexcept
{
    score::td::svt::TimeBaseSnapshot snapshot{};
    snapshot.ptp_assumed_time = 1000ULL;
    snapshot.local_time = 0ULL;
    snapshot.rate_deviation = rate_deviation;
    snapshot.status = status;
    return snapshot;
}

/// Returns whatever snapshot the test serves and counts how often it was polled.
class FakeSvtReceiver final : public score::td::SvtReceiver
{
  public:
    void SetInitResult(const bool ok) noexcept
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        init_result_ = ok;
    }

    std::size_t InitCalls() const noexcept
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        return init_calls_;
    }

    void Serve(const std::optional<score::td::svt::TimeBaseSnapshot>& snapshot) noexcept
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        snapshot_ = snapshot;
    }

    std::size_t Polls() const noexcept
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        return polls_;
    }

    [[nodiscard]] bool WaitForPolls(const std::size_t additional) noexcept
    {
        std::unique_lock<std::mutex> lock{mutex_};
        const std::size_t target = polls_ + additional;
        return polled_.wait_for(lock, kWaitTimeout, [this, target]() noexcept {
            return polls_ >= target;
        });
    }

    bool Init() noexcept override
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        ++init_calls_;
        return init_result_;
    }

    std::optional<score::td::svt::TimeBaseSnapshot> Receive() noexcept override
    {
        // Notify under the lock so the object can be destroyed as soon as a waiter resumes.
        const std::lock_guard<std::mutex> lock{mutex_};
        ++polls_;
        polled_.notify_all();
        return snapshot_;
    }

  private:
    mutable std::mutex mutex_;
    std::condition_variable polled_;
    std::optional<score::td::svt::TimeBaseSnapshot> snapshot_{};
    std::size_t polls_{0U};
    std::size_t init_calls_{0U};
    bool init_result_{true};
};

/// Captures callback invocations so the test thread can wait for them and inspect the last one.
template <typename Event>
class Recorder
{
  public:
    auto Callback() noexcept
    {
        return [this](const Event& event) noexcept {
            Record(event);
        };
    }

    void Record(const Event& event) noexcept
    {
        // Notify under the lock so the recorder can be destroyed as soon as a waiter resumes.
        const std::lock_guard<std::mutex> lock{mutex_};
        last_ = event;
        ++count_;
        recorded_.notify_all();
    }

    [[nodiscard]] bool WaitForCount(const std::size_t count) noexcept
    {
        std::unique_lock<std::mutex> lock{mutex_};
        return recorded_.wait_for(lock, kWaitTimeout, [this, count]() noexcept {
            return count_ >= count;
        });
    }

    std::size_t Count() const noexcept
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        return count_;
    }

    Event Last() const noexcept
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        return last_;
    }

  private:
    mutable std::mutex mutex_;
    std::condition_variable recorded_;
    Event last_{};
    std::size_t count_{0U};
};

}  // namespace test_helpers
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_VEHICLE_TIME_SRC_DETAILS_TD_IMPL_SVT_TEST_HELPERS_H
