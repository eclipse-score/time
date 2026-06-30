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
#include "score/time_daemon/src/ptp_machine/stub/factory.h"
#include "score/time_daemon/src/ptp_machine/stub/details/stub_ptp_engine.h"
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <mutex>
#include <thread>

namespace score
{
namespace td
{

class GPTPStubMachineIntegrationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        machine_ = CreateGPTPStubMachine("StubPTPMachine");

        machine_->SetPublishCallback([this](const PtpTimeInfo& data) {
            {
                std::lock_guard<std::mutex> lock(publish_data_guard_);
                published_data_ = data;
            }
            promise_data_published_.set_value();
        });
    }

    void TearDown() override
    {
        machine_->Stop();
        machine_.reset();
    }

    std::shared_ptr<GPTPStubMachine> machine_;
    std::promise<void> promise_data_published_;
    PtpTimeInfo published_data_;
    std::mutex publish_data_guard_;
};

TEST_F(GPTPStubMachineIntegrationTest, ConstructionTest)
{
    EXPECT_EQ(machine_->GetName(), "StubPTPMachine");
}

TEST_F(GPTPStubMachineIntegrationTest, InitializationTest)
{
    EXPECT_TRUE(machine_->Init());
}

TEST_F(GPTPStubMachineIntegrationTest, GetSynchronizedDataTest)
{
    EXPECT_TRUE(machine_->Init());

    auto clock = score::time::HighResSteadyClock::GetInstance();
    const auto time_before_start = clock.Now().TimeSinceEpoch();

    machine_->Start();
    auto future = promise_data_published_.get_future();
    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(300)), std::future_status::ready);

    const auto time_after_publish = clock.Now().TimeSinceEpoch();

    {
        std::lock_guard<std::mutex> lock(publish_data_guard_);

        // PTP assumed time should be derived from the local clock and lie within the measured window
        EXPECT_GE(published_data_.ptp_assumed_time, time_before_start);
        EXPECT_LE(published_data_.ptp_assumed_time, time_after_publish);

        // Stub always reports synchronized and correct status
        EXPECT_TRUE(published_data_.status.is_synchronized);
        EXPECT_FALSE(published_data_.status.is_timeout);
        EXPECT_FALSE(published_data_.status.is_time_jump_future);
        EXPECT_FALSE(published_data_.status.is_time_jump_past);
        EXPECT_TRUE(published_data_.status.is_correct);

        // Rate deviation is always zero in the stub
        EXPECT_DOUBLE_EQ(published_data_.rate_deviation, 0.0);

        // Sync measurement data should be populated with consistent timestamps
        EXPECT_GT(published_data_.sync_fup_data.precise_origin_timestamp, 0U);
        EXPECT_EQ(published_data_.sync_fup_data.precise_origin_timestamp,
                  published_data_.sync_fup_data.reference_global_timestamp);
        EXPECT_EQ(published_data_.sync_fup_data.reference_global_timestamp,
                  published_data_.sync_fup_data.reference_local_timestamp);
        EXPECT_EQ(published_data_.sync_fup_data.reference_local_timestamp,
                  published_data_.sync_fup_data.sync_ingress_timestamp);
        EXPECT_EQ(published_data_.sync_fup_data.correction_field, 0U);
        EXPECT_EQ(published_data_.sync_fup_data.pdelay, 1'000U);
        EXPECT_EQ(published_data_.sync_fup_data.port_number, 1U);
        EXPECT_EQ(published_data_.sync_fup_data.clock_identity, 0xAABBCCDDEEFF0011ULL);

        // PDelay measurement data should be populated with consistent timestamps
        constexpr std::uint64_t kExpectedOnewayDelayNs{1'000U};
        EXPECT_GT(published_data_.pdelay_data.request_origin_timestamp, 0U);
        EXPECT_EQ(published_data_.pdelay_data.request_receipt_timestamp,
                  published_data_.pdelay_data.request_origin_timestamp + kExpectedOnewayDelayNs);
        EXPECT_EQ(published_data_.pdelay_data.response_origin_timestamp,
                  published_data_.pdelay_data.request_origin_timestamp + kExpectedOnewayDelayNs);
        EXPECT_EQ(published_data_.pdelay_data.response_receipt_timestamp,
                  published_data_.pdelay_data.request_origin_timestamp + 2U * kExpectedOnewayDelayNs);
        EXPECT_EQ(published_data_.pdelay_data.pdelay, kExpectedOnewayDelayNs);
        EXPECT_EQ(published_data_.pdelay_data.req_port_number, 1U);
        EXPECT_EQ(published_data_.pdelay_data.req_clock_identity, 0xAABBCCDDEEFF0011ULL);
        EXPECT_EQ(published_data_.pdelay_data.resp_port_number, 2U);
        EXPECT_EQ(published_data_.pdelay_data.resp_clock_identity, 0x1122334455667788ULL);
    }
}

}  // namespace td
}  // namespace score
