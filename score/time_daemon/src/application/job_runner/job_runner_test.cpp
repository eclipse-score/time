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
#include "score/time_daemon/src/application/job_runner/job_runner.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

namespace score
{
namespace td
{

/**
 * @brief Test fixture for JobRunner tests.
 */
class JobRunnerTest : public ::testing::Test
{
  protected:
    score::cpp::stop_source stop_source;
    score::cpp::stop_token token = stop_source.get_token();

    /**
     * @brief Wait until JobRunner finishes or timeout occurs.
     */
    JobRunner::Result WaitForCompletion(JobRunner& runner, std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        auto start = std::chrono::steady_clock::now();
        JobRunner::Result result = runner.GetResult();
        while (result == JobRunner::Result::kInProgress && std::chrono::steady_clock::now() - start < timeout)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            result = runner.GetResult();
        }
        return result;
    }
};

/**
 * @brief Test that a job which succeeds immediately results in kSucceed.
 */
TEST_F(JobRunnerTest, JobSucceedsImmediately)
{
    Job job{[]() {
                return true;
            },
            "success_job",
            std::chrono::seconds(1)};

    JobRunner runner({job}, "test");
    runner.Start(token);

    JobRunner::Result result = WaitForCompletion(runner);
    EXPECT_EQ(result, JobRunner::Result::kSucceed);
}

/**
 * @brief Test that a job that never succeeds will time out and result in kFailed.
 */
TEST_F(JobRunnerTest, JobTimesOut)
{
    Job job{[]() {
                return false;
            },
            "timeout_job",
            std::chrono::seconds(1)};

    JobRunner runner({job}, "test");
    runner.Start(token);

    JobRunner::Result result = WaitForCompletion(runner);
    EXPECT_EQ(result, JobRunner::Result::kFailed);
}

/**
 * @brief Test that GetResult returns kInProgress while jobs are still running.
 */
TEST_F(JobRunnerTest, ResultWhileRunning)
{
    std::atomic<int> counter{0};

    Job job{[&counter]() {
                counter++;
                return false;  // never succeeds
            },
            "long_job",
            std::chrono::seconds(2)};

    JobRunner runner({job}, "test");
    runner.Start(token);

    // Give thread a chance to start
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_EQ(runner.GetResult(), JobRunner::Result::kInProgress);

    JobRunner::Result result = WaitForCompletion(runner);
    EXPECT_EQ(result, JobRunner::Result::kFailed);
    EXPECT_GT(counter.load(), 0);  // function was called at least once
}

/**
 * @brief Test stopping a running job early.
 */
TEST_F(JobRunnerTest, StopJobRunnerEarly)
{
    std::atomic<int> counter{0};

    Job job{[&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                counter++;
                return false;
            },
            "long_job",
            std::chrono::seconds(5)};

    JobRunner runner({job}, "test");
    runner.Start(token);

    // Give thread a chance to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(runner.GetResult(), JobRunner::Result::kInProgress);

    // Request stop
    stop_source.request_stop();

    // Wait for status update after stop
    JobRunner::Result result = WaitForCompletion(runner);
    EXPECT_EQ(result, JobRunner::Result::kFailed);
    EXPECT_LE(counter.load(), 1);  // function may or may not have executed
}

/**
 * @brief Test stopping multiple jobs early.
 */
TEST_F(JobRunnerTest, StopMultipleJobsEarly)
{
    std::atomic<int> counter{0};

    Job job1{[&counter]() {
                 std::this_thread::sleep_for(std::chrono::milliseconds(50));
                 counter++;
                 return false;
             },
             "job1",
             std::chrono::seconds(5)};
    Job job2{[&counter]() {
                 std::this_thread::sleep_for(std::chrono::milliseconds(50));
                 counter++;
                 return false;
             },
             "job2",
             std::chrono::seconds(5)};

    JobRunner runner({job1, job2}, "test");
    runner.Start(token);

    // Give thread a chance to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(runner.GetResult(), JobRunner::Result::kInProgress);

    // Request stop
    stop_source.request_stop();

    // Wait for status update after stop
    JobRunner::Result result = WaitForCompletion(runner);
    EXPECT_EQ(result, JobRunner::Result::kFailed);
    EXPECT_LE(counter.load(), 2);
}

}  // namespace td
}  // namespace score
