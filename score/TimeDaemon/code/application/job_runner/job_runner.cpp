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
#include "score/TimeDaemon/code/application/job_runner/job_runner.h"
#include "score/TimeDaemon/code/common/logging_contexts.h"
#include "score/concurrency/interruptible_wait.h"
#include "score/mw/log/logging.h"

namespace score
{
namespace td
{

JobRunner::JobRunner(std::vector<Job> jobs, const std::string name)
    : jobs_(std::move(jobs)), name_(name), status_{Result::kIdle}
{
}

void JobRunner::Start(const score::cpp::stop_token& token)
{
    {
        std::lock_guard<std::mutex> lock(status_mutex_);
        if (status_ != Result::kIdle)
        {
            return;  // Already running
        }

        // Set before score::cpp::jthread as if we will set it after, worker_thread_
        // it could be already done.
        status_ = Result::kInProgress;
    }

    const auto thread_name = "td_" + name_ + "_worker";
    worker_thread_ = score::cpp::jthread(score::cpp::jthread::name_hint{thread_name}, [this, &token]() {
        bool success = RunJobs(token);
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            status_ = success ? Result::kSucceed : Result::kFailed;
        }
    });
}

bool JobRunner::RunJobs(const score::cpp::stop_token& token)
{
    bool all_success = true;

    const auto current_timepoint = std::chrono::steady_clock::now();
    for (auto& job : jobs_)
    {
        job.start = current_timepoint;
    }

    while (!token.stop_requested() && !jobs_.empty())
    {
        for (auto it = jobs_.begin(); it != jobs_.end();)
        {
            auto elapsed = std::chrono::steady_clock::now() - it->start;

            if (elapsed >= it->timeout)
            {
                score::mw::log::LogError(kTimeBaseHandlerSvt) << it->name << " timed out";
                all_success = false;
                it = jobs_.erase(it);
            }
            else if (it->fn())
            {
                score::mw::log::LogInfo(kTimeBaseHandlerSvt) << it->name << " initialized successfully";
                it = jobs_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (!jobs_.empty())
            score::concurrency::wait_for(token, std::chrono::milliseconds(10));
    }
    // If the loop exited due to stop token, mark as failure
    if (token.stop_requested())
    {
        all_success = false;
    }

    return all_success;
}

JobRunner::Result JobRunner::GetResult() const
{
    std::lock_guard<std::mutex> lock(status_mutex_);
    return status_;
}

}  // namespace td
}  // namespace score
