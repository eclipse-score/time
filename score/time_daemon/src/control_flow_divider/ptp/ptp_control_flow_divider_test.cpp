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
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/control_flow_divider/ptp/factory.h"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>
#include <mutex>

using namespace std::chrono_literals;

namespace score
{
namespace td
{

class PtpControlFlowDividerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        divider_ = CreatePtpControlFlowDivider("TestPtpDivider", 100ms);

        expected_published_data_count_ = 0U;
        published_data_.clear();
        divider_->SetPublishCallback([this](const PtpTimeInfo& data) {
            auto published_data_count = 0U;
            {
                std::lock_guard<std::mutex> lock(publish_data_guard_);
                published_data_.push_back(data);
                published_data_count = published_data_.size();
            }
            if (expected_published_data_count_ != 0U && published_data_count >= expected_published_data_count_)
            {
                promise_data_published_.set_value();
            }
        });
    }

    void TearDown() override
    {
        divider_->Stop();
        divider_.reset();
    }

    void StopAfterReceivingAmount(size_t amount)
    {
        expected_published_data_count_ = amount;
    }

    std::shared_ptr<PtpControlFlowDivider> divider_{nullptr};

    size_t expected_published_data_count_;
    std::promise<void> promise_data_published_;
    std::vector<PtpTimeInfo> published_data_;
    std::mutex publish_data_guard_;
};

TEST_F(PtpControlFlowDividerTest, TestNormalFunctionality)
{
    std::chrono::nanoseconds ptp_assumed_time{54321};
    PtpTimeInfo::ReferenceClock::time_point local_time{std::chrono::nanoseconds{12345}};

    PtpTimeInfo expected_data = {ptp_assumed_time, local_time, 0.0, {}, {}, {}};
    expected_data.status.is_synchronized = 1U;

    StopAfterReceivingAmount(1U);

    EXPECT_TRUE(divider_->Init());
    divider_->Start();

    // Send data through the divider
    divider_->OnMessage(expected_data);

    // Wait for data to be published
    auto future = promise_data_published_.get_future();
    ASSERT_EQ(future.wait_for(300ms), std::future_status::ready);

    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);
        // Verify data was correctly passed through
        ASSERT_EQ(published_data_.size(), 1);
        const auto& received_data = published_data_.front();
        EXPECT_EQ(received_data.local_time, expected_data.local_time);
        EXPECT_EQ(received_data.ptp_assumed_time, expected_data.ptp_assumed_time);
        EXPECT_EQ(received_data.status.is_synchronized, expected_data.status.is_synchronized);
    }
}

TEST_F(PtpControlFlowDividerTest, TestNormalFunctionalityWithMultipleMessages)
{
    const size_t kMessageCount = 5U;

    EXPECT_TRUE(divider_->Init());
    divider_->Start();

    // Simulate PTP Machine sending data at regular intervals
    for (size_t i = 0; i < kMessageCount; ++i)
    {
        PtpTimeInfo data{};

        PtpTimeInfo::ReferenceClock::time_point cur_local_time{std::chrono::nanoseconds{1000U * (i + 1)}};
        std::chrono::nanoseconds cur_ptp_time{2000U * (i + 1)};
        data.local_time = cur_local_time;
        data.ptp_assumed_time = cur_ptp_time;
        data.status.is_synchronized = 1U;
        data.rate_deviation = 0.0;

        divider_->OnMessage(data);
        std::this_thread::sleep_for(50ms);
    }

    // Wait for all data to be published
    auto future = promise_data_published_.get_future();
    EXPECT_EQ(future.wait_for(500ms), std::future_status::timeout);

    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);
        EXPECT_GE(published_data_.size(), kMessageCount);

        size_t idx = 0U;
        PtpTimeInfo cur_data{};
        for (; idx < kMessageCount; ++idx)
        {
            cur_data = published_data_[idx];
            EXPECT_EQ(cur_data.local_time.time_since_epoch().count(), 1000U * (idx + 1));
            EXPECT_EQ(cur_data.ptp_assumed_time.count(), 2000U * (idx + 1));
            EXPECT_EQ(cur_data.status.is_synchronized, 1U);
        }

        // the rest should be the same as the last one
        for (; idx < published_data_.size(); ++idx)
        {
            EXPECT_EQ(published_data_[idx].local_time, cur_data.local_time);
            EXPECT_EQ(published_data_[idx].ptp_assumed_time, cur_data.ptp_assumed_time);
            EXPECT_EQ(published_data_[idx].status.is_synchronized, cur_data.status.is_synchronized);
        }
    }
}

TEST_F(PtpControlFlowDividerTest, TestTimeoutBehavior)
{
    EXPECT_TRUE(divider_->Init());
    divider_->Start();

    // Don't send any data - should timeout and publish empty data

    auto future = promise_data_published_.get_future();
    ASSERT_EQ(future.wait_for(300ms), std::future_status::timeout);

    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);
        EXPECT_GE(published_data_.size(), 1);

        // Verify empty data (all zeros)
        const PtpTimeInfo empty_data{};
        for (const auto& cur_data : published_data_)
        {
            EXPECT_EQ(cur_data, empty_data);
        }
    }
}

}  // namespace td
}  // namespace score
