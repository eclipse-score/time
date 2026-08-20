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
        pub_.Close();
    }

    std::string name_;
    GptpIpcPublisher pub_;
    GptpIpcReceiver rx_;
};

TEST_F(GptpIpcRoundtripTest, ReceiverOpen_AfterPublisherOpen_ReturnsTrue)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__publisher_creates");
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__receiver_multi_reader");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty("Description",
                                    "Receiver::Open succeeds after Publisher creates shared memory segment.");

    ASSERT_TRUE(pub_.Open(name_));
    EXPECT_TRUE(rx_.Open(name_));
}

TEST_F(GptpIpcRoundtripTest, ReceiverReceive_BeforeAnyPublish_ReturnsNullopt)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__data_validity");
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "boundary-values");
    ::testing::Test::RecordProperty(
        "Description", "Receiver::Receive before any Publish returns empty optional due to seqlock mismatch.");

    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(rx_.Open(name_));
    // seq_confirm is initialised to 1 (≠ seq=0) by GptpIpcRegion's constructor,
    // so the seqlock always mismatches before the first Publish() call.
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_BasicFields_RoundtripCorrectly)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__time_correlation_data");
    ::testing::Test::RecordProperty("PartiallyVerifies",
                                    "comp_req__ts_client__sync_status_data,comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty(
        "Description",
        "Publisher::Publish and Receiver::Receive correctly exchange time correlation data and status fields.");

    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(rx_.Open(name_));

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
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__sync_status_data");
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "boundary-values");
    ::testing::Test::RecordProperty(
        "Description",
        "Publisher::Publish and Receiver::Receive correctly exchange all gPTP synchronization status flags.");

    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(rx_.Open(name_));

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
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__sync_fup_data");
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty(
        "Description",
        "Publisher::Publish and Receiver::Receive correctly exchange Sync/FollowUp message metadata fields.");

    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(rx_.Open(name_));

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
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__pdelay_data");
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty(
        "Description", "Publisher::Publish and Receiver::Receive correctly exchange PDelay message metadata fields.");

    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(rx_.Open(name_));

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
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty(
        "Description", "Multiple Publisher::Publish calls result in Receiver::Receive returning last published value.");

    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(rx_.Open(name_));

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

TEST_F(GptpIpcRoundtripTest, ReceiverOpen_WrongMagic_ReturnsFalse)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__shm_validation");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "boundary-values");
    ::testing::Test::RecordProperty(
        "Description", "Receiver::Open with incorrect magic number in shared memory region returns false.");

    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    auto* region = new (shm.Region()) GptpIpcRegion{};
    const std::uint32_t bad = 0xDEADBEEFU;
    region->magic.store(bad, std::memory_order_release);

    EXPECT_FALSE(rx_.Open(name_));
}

TEST_F(GptpIpcRoundtripTest, Receive_PersistentOddSeq_ExhaustsRetriesAndReturnsNullopt)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__data_validity");
    ::testing::Test::RecordProperty("TestType", "fault-injection");
    ::testing::Test::RecordProperty("DerivationTechnique", "error-guessing");
    ::testing::Test::RecordProperty(
        "Description",
        "Receiver::Receive with persistent odd sequence counter exhausts retries and returns empty optional.");

    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    auto* region = new (shm.Region()) GptpIpcRegion{};
    region->seq.store(1U, std::memory_order_relaxed);
    region->seq_confirm.store(0U, std::memory_order_relaxed);

    ASSERT_TRUE(rx_.Open(name_));
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcRoundtripTest, Receive_SeqConfirmMismatch_ExhaustsRetriesAndReturnsNullopt)
{
    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    auto* region = new (shm.Region()) GptpIpcRegion{};
    region->seq.store(4U, std::memory_order_relaxed);
    region->seq_confirm.store(2U, std::memory_order_relaxed);

    ASSERT_TRUE(rx_.Open(name_));
    EXPECT_FALSE(rx_.Receive().has_value());
}

}  // namespace details
}  // namespace ts
}  // namespace score
