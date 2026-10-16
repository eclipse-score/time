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
#include "score/time_daemon/src/common/machines/event_driven_machine.h"
#include "score/stop_token.hpp"
#include "score/time_daemon/src/common/machines/proactive_machine.h"
#include "score/utility.hpp"
#include <chrono>
#include <mutex>
#include <string>

namespace score::td
{

EventDrivenMachine::EventDrivenMachine(const std::string& name, const std::chrono::milliseconds timeout)
    : ProactiveMachine(name), kTimeout_(timeout)
{
}

void EventDrivenMachine::Start() noexcept
{
    const auto thread_name = "td_" + GetName() + "_worker";
    worker_ = score::cpp::jthread{score::cpp::jthread::name_hint{thread_name},
                                  [this](const score::cpp::stop_token& token) noexcept {
                                      WorkerFunction(token);
                                  }};
}

// join()/notify_one() escaping here means shutdown is broken beyond recovery; terminating via
// noexcept is the intended behavior, not swallowed here.
// NOLINTNEXTLINE(bugprone-exception-escape)
void EventDrivenMachine::Stop() noexcept
{
    if (worker_.joinable())
    {
        {
            const std::lock_guard<std::mutex> guard{cv_mutex_};
            score::cpp::ignore = worker_.request_stop();
            cv_.notify_one();
        }

        worker_.join();
    }
}

void EventDrivenMachine::NotifyEvent() noexcept
{
    const std::lock_guard<std::mutex> guard{cv_mutex_};
    event_pending_ = true;
    cv_.notify_one();
}

void EventDrivenMachine::WorkerFunction(const score::cpp::stop_token& stop_token) noexcept
{
    while (!stop_token.stop_requested())
    {
        bool event_occurred = false;

        {
            std::unique_lock<std::mutex> lock{cv_mutex_};
            const bool was_interrupted =
                cv_.wait_for(lock, stop_token, kTimeout_, [&stop_token, this]() noexcept -> bool {
                    return stop_token.stop_requested() || event_pending_;
                });

            if (was_interrupted)
            {
                event_occurred = event_pending_;
                event_pending_ = false;
            }
        }

        if (stop_token.stop_requested())
        {
            break;
        }

        if (event_occurred)
        {
            OnEvent();
        }
        else
        {
            OnTimeout();
        }
    }
}

}  // namespace score::td
