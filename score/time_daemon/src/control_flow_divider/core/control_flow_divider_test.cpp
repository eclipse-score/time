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
#include "score/time_daemon/src/control_flow_divider/core/control_flow_divider.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

namespace score
{
namespace td
{

using ::testing::_;
using ::testing::Invoke;

namespace testing
{

struct TestData
{
    size_t value;
};

inline bool operator==(const TestData& lhs, const TestData& rhs)
{
    return lhs.value == rhs.value;
}

template <typename OutputStream>
inline auto& operator<<(OutputStream& os, const TestData& data)
{
    return os << "TestData(value=" << data.value << ")";
}

}  // namespace testing

class ControlFlowDividerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        divider_ = std::make_unique<ControlFlowDividerType>("TestDivider", 100ms);

        expected_published_data_count_ = 0U;
        published_data_.clear();
        divider_->SetPublishCallback([this](const testing::TestData& data) {
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

    using ControlFlowDividerType = ControlFlowDivider<testing::TestData, 10>;
    std::unique_ptr<ControlFlowDividerType> divider_;

    size_t expected_published_data_count_;
    std::promise<void> promise_data_published_;
    std::vector<testing::TestData> published_data_;
    std::mutex publish_data_guard_;
};

TEST_F(ControlFlowDividerTest, TestInitialization)
{
    EXPECT_TRUE(divider_->Init());
    EXPECT_EQ(divider_->GetName(), "TestDivider");
}

TEST_F(ControlFlowDividerTest, TestPublishValidData)
{
    testing::TestData expected_data{};
    expected_data.value = 123U;

    StopAfterReceivingAmount(1U);

    EXPECT_TRUE(divider_->Init());

    divider_->Start();

    divider_->OnMessage(expected_data);

    // Wait for data to be published
    auto future = promise_data_published_.get_future();
    EXPECT_EQ(future.wait_for(3000ms), std::future_status::ready);

    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);
        EXPECT_EQ(published_data_.size(), 1);
        EXPECT_EQ(published_data_.front(), expected_data);
    }
}

TEST_F(ControlFlowDividerTest, TestPublishEmptyDataOnTimeout)
{
    EXPECT_TRUE(divider_->Init());
    divider_->Start();

    // Don't send any data, wait for timeout

    auto future = promise_data_published_.get_future();
    EXPECT_EQ(future.wait_for(500ms), std::future_status::timeout);

    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);
        EXPECT_GE(published_data_.size(), 1);

        // Verify empty data (all zeros)
        testing::TestData empty_data{};
        for (const auto& cur_data : published_data_)
        {
            EXPECT_EQ(cur_data, empty_data);
        }
    }
}

TEST_F(ControlFlowDividerTest, TestMultipleDataPublish)
{
    constexpr size_t kExpectedCount = 3U;
    StopAfterReceivingAmount(kExpectedCount);

    EXPECT_TRUE(divider_->Init());
    divider_->Start();

    // Send multiple data items
    for (auto i = 0U; i < kExpectedCount; ++i)
    {
        testing::TestData data{};
        data.value = 100U + i;
        divider_->OnMessage(data);

        std::this_thread::sleep_for(50ms);
    }

    auto future = promise_data_published_.get_future();
    EXPECT_EQ(future.wait_for(500ms), std::future_status::ready);

    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);
        EXPECT_EQ(published_data_.size(), kExpectedCount);
    }
}

TEST_F(ControlFlowDividerTest, TestNormalQueueBehavior)
{
    constexpr size_t kExpectedCount = 3U;
    StopAfterReceivingAmount(kExpectedCount);

    EXPECT_TRUE(divider_->Init());
    divider_->Start();

    // Queue multiple items before starting
    testing::TestData data1{};
    data1.value = 100U;
    divider_->OnMessage(data1);

    testing::TestData data2{};
    data2.value = 200U;
    divider_->OnMessage(data2);

    testing::TestData data3{};
    data3.value = 300U;
    divider_->OnMessage(data3);

    // Wait for all data to be published
    auto future = promise_data_published_.get_future();
    EXPECT_EQ(future.wait_for(1000ms), std::future_status::ready);

    // Verify FIFO order
    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);
        EXPECT_GE(published_data_.size(), kExpectedCount);
        EXPECT_EQ(published_data_[0].value, 100U);
        EXPECT_EQ(published_data_[1].value, 200U);
        EXPECT_EQ(published_data_[2].value, 300U);
    }
}

}  // namespace td
}  // namespace score
