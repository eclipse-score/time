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
#ifndef SCORE_TIMEDAEMON_CODE_APPLICATION_JOB_RUNNER_JOB_RUNNER_H
#define SCORE_TIMEDAEMON_CODE_APPLICATION_JOB_RUNNER_JOB_RUNNER_H

#include <score/jthread.hpp>
#include <score/stop_token.hpp>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace score
{
namespace td
{

/**
 * @brief Represents a single job with a function, name, timeout, and start time.
 */
struct Job
{
    std::function<bool()> fn;
    std::string name;
    std::chrono::seconds timeout;
    std::chrono::steady_clock::time_point start{};
};

/**
 * @brief Runs multiple jobs asynchronously with timeouts.
 *
 * JobRunner manages a collection of jobs, runs them asynchronously,
 * and supports polling for completion.
 */
class JobRunner
{
  public:
    /**
     * @brief Construct a new JobRunner object.
     *
     * @param jobs Vector of jobs to run.
     * @param name name of the job.
     */
    JobRunner(std::vector<Job> jobs, const std::string name);

    /**
     * @brief Represents the jobs status
     */
    enum class Result
    {
        kIdle,
        kInProgress,
        kSucceed,
        kFailed
    };

    /**
     * @brief Start executing the jobs asynchronously.
     *
     * @param token Stop token used to cancel execution.
     */
    void Start(const score::cpp::stop_token& token);

    /**
     * @brief Reads the result of the running job
     *
     * @return enum Result
     */
    Result GetResult() const;

  private:
    /**
     * @brief Executes all jobs in the JobRunner asynchronously until completion or timeout.
     *
     * @param token A stop token that can request early termination of job execution.
     * @return true if all jobs completed successfully; false if any job failed or timed out.
     */
    bool RunJobs(const score::cpp::stop_token& token);

    std::vector<Job> jobs_;
    const std::string name_;
    Result status_;
    mutable std::mutex status_mutex_;
    score::cpp::jthread worker_thread_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_APPLICATION_JOB_RUNNER_JOB_RUNNER_H
