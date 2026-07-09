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
#include "score/time_daemon/src/common/machines/periodic_machine.h"

namespace score
{
namespace td
{

PeriodicMachine::PeriodicMachine(const std::string& name, const std::chrono::milliseconds threadCycle)
    : ProactiveMachine(name), cv_mutex_{}, cv_{}, worker_{}, kCycleTime_(threadCycle)
{
}

void PeriodicMachine::Start() noexcept
{
    const auto thread_name = "td_" + GetName() + "_worker";
    worker_ = score::cpp::jthread{score::cpp::jthread::name_hint{thread_name},
                                  [this](const score::cpp::stop_token token) noexcept {
                                      WorkerFunction(token);
                                  }};
}

void PeriodicMachine::Stop() noexcept
{
    if (worker_.joinable())
    {
        {
            std::lock_guard<std::mutex> guard{cv_mutex_};
            score::cpp::ignore = worker_.request_stop();
            cv_.notify_one();
        }

        worker_.join();
    }
}

void PeriodicMachine::WorkerFunction(const score::cpp::stop_token& stop_token) noexcept
{
    while (!stop_token.stop_requested())
    {
        // Execute periodic task
        PeriodicTask();

        // Wait for next cycle
        {
            std::unique_lock<std::mutex> lock_mutex{cv_mutex_};
            score::cpp::ignore = cv_.wait_for(lock_mutex, stop_token, kCycleTime_, [&stop_token]() noexcept -> bool {
                return stop_token.stop_requested();
            });
        }
    }
}

}  // namespace td
}  // namespace score
