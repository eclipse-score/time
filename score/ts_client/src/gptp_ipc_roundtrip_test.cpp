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
#include "score/ts_client/src/gptp_ipc_publisher.h"
#include "score/ts_client/src/gptp_ipc_receiver.h"
#include "score/ts_client/src/gptp_ipc_test_utils.h"

#include <gtest/gtest.h>

#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

class GptpIpcRoundtripTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        name_ = UniqueShmName();
    }
    void TearDown() override
    {
        rx_.Close();
        pub_.Destroy();
    }

    std::string name_;
    GptpIpcPublisher pub_;
    GptpIpcReceiver rx_;
};

TEST_F(GptpIpcRoundtripTest, ReceiverInit_AfterPublisherInit_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    EXPECT_TRUE(rx_.Init(name_));
}

TEST_F(GptpIpcRoundtripTest, ReceiverReceive_BeforeAnyPublish_ReturnsNullopt)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));
    // seq_confirm is initialised to 1 (≠ seq=0) by GptpIpcRegion's constructor,
    // so the seqlock always mismatches before the first Publish() call.
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_BasicFields_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::ts::GptpIpcData data{};
    data.ptp_assumed_time = std::chrono::nanoseconds{1'234'567'890LL};
    data.rate_deviation = 0.75;
    data.status.is_synchronized = true;
    data.status.is_correct = true;

    pub_.Publish(data);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ptp_assumed_time, data.ptp_assumed_time);
    EXPECT_DOUBLE_EQ(result->rate_deviation, data.rate_deviation);
    EXPECT_TRUE(result->status.is_synchronized);
    EXPECT_TRUE(result->status.is_correct);
    EXPECT_FALSE(result->status.is_timeout);
    EXPECT_FALSE(result->status.is_time_jump_future);
    EXPECT_FALSE(result->status.is_time_jump_past);
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_StatusFlags_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::ts::GptpIpcData data{};
    data.status.is_timeout = true;
    data.status.is_time_jump_future = true;
    data.status.is_synchronized = false;
    data.status.is_correct = false;

    pub_.Publish(data);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->status.is_timeout);
    EXPECT_TRUE(result->status.is_time_jump_future);
    EXPECT_FALSE(result->status.is_time_jump_past);
    EXPECT_FALSE(result->status.is_synchronized);
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_SyncFupData_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::ts::GptpIpcData data{};
    data.sync_fup_data.precise_origin_timestamp = 100'000'000'000ULL;
    data.sync_fup_data.reference_global_timestamp = 100'000'001'000ULL;
    data.sync_fup_data.reference_local_timestamp = 100'000'001'500ULL;
    data.sync_fup_data.sync_ingress_timestamp = 100'000'001'500ULL;
    data.sync_fup_data.correction_field = 42U;
    data.sync_fup_data.sequence_id = 77;
    data.sync_fup_data.pdelay = 3'000U;
    data.sync_fup_data.port_number = 1;
    data.sync_fup_data.clock_identity = 0xAABBCCDDEEFF0011ULL;

    pub_.Publish(data);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->sync_fup_data.precise_origin_timestamp, 100'000'000'000ULL);
    EXPECT_EQ(result->sync_fup_data.reference_global_timestamp, 100'000'001'000ULL);
    EXPECT_EQ(result->sync_fup_data.sequence_id, 77);
    EXPECT_EQ(result->sync_fup_data.pdelay, 3'000U);
    EXPECT_EQ(result->sync_fup_data.clock_identity, 0xAABBCCDDEEFF0011ULL);
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_PDelayData_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::ts::GptpIpcData data{};
    data.pdelay_data.request_origin_timestamp = 1'000'000'000ULL;
    data.pdelay_data.request_receipt_timestamp = 1'000'001'000ULL;
    data.pdelay_data.response_origin_timestamp = 1'000'001'000ULL;
    data.pdelay_data.response_receipt_timestamp = 1'000'002'000ULL;
    data.pdelay_data.pdelay = 1'000U;
    data.pdelay_data.req_port_number = 1;
    data.pdelay_data.resp_port_number = 2;
    data.pdelay_data.req_clock_identity = 0x1122334455667788ULL;

    pub_.Publish(data);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pdelay_data.request_origin_timestamp, 1'000'000'000ULL);
    EXPECT_EQ(result->pdelay_data.pdelay, 1'000U);
    EXPECT_EQ(result->pdelay_data.req_port_number, 1);
    EXPECT_EQ(result->pdelay_data.resp_port_number, 2);
    EXPECT_EQ(result->pdelay_data.req_clock_identity, 0x1122334455667788ULL);
}

TEST_F(GptpIpcRoundtripTest, MultiplePublish_LastValueIsVisible)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    for (int i = 1; i <= 5; ++i)
    {
        score::ts::GptpIpcData data{};
        data.ptp_assumed_time = std::chrono::nanoseconds{static_cast<std::int64_t>(i) * 1'000'000'000LL};
        pub_.Publish(data);
    }

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ptp_assumed_time, std::chrono::nanoseconds{5'000'000'000LL});
}

// ── Edge cases via ManualShm ──────────────────────────────────────────────────

TEST_F(GptpIpcRoundtripTest, ReceiverInit_WrongMagic_ReturnsFalse)
{
    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    new (shm.Region()) GptpIpcRegion{};
    const std::uint32_t bad = 0xDEADBEEFU;
    std::memcpy(shm.ptr, &bad, sizeof(bad));

    EXPECT_FALSE(rx_.Init(name_));
}

TEST_F(GptpIpcRoundtripTest, Receive_PersistentOddSeq_ExhaustsRetriesAndReturnsNullopt)
{
    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    auto* region = new (shm.Region()) GptpIpcRegion{};
    region->seq.store(1U, std::memory_order_relaxed);
    region->seq_confirm.store(0U, std::memory_order_relaxed);

    ASSERT_TRUE(rx_.Init(name_));
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcRoundtripTest, Receive_SeqConfirmMismatch_ExhaustsRetriesAndReturnsNullopt)
{
    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    auto* region = new (shm.Region()) GptpIpcRegion{};
    region->seq.store(4U, std::memory_order_relaxed);
    region->seq_confirm.store(2U, std::memory_order_relaxed);

    ASSERT_TRUE(rx_.Init(name_));
    EXPECT_FALSE(rx_.Receive().has_value());
}

}  // namespace details
}  // namespace ts
}  // namespace score
