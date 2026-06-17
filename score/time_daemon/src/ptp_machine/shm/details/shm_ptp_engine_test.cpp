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
#include "score/time_daemon/src/ptp_machine/shm/details/shm_ptp_engine.h"

#include "score/ts_client/src/gptp_ipc_publisher.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>

namespace score
{
namespace td
{
namespace details
{

namespace
{

std::string UniqueShmName()
{
    static std::atomic<int> counter{0};
    return "/gptp_shm_ut_" + std::to_string(::getpid()) + "_" +
           std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

/// Build a fully-populated GptpIpcData for roundtrip verification.
score::ts::GptpIpcData MakeTestIpcData()
{
    score::ts::GptpIpcData d{};
    d.ptp_assumed_time = std::chrono::nanoseconds{9'876'543'210LL};
    d.local_time = std::chrono::nanoseconds{42'000'000'000LL};
    d.rate_deviation = -0.25;

    d.status.is_synchronized = true;
    d.status.is_correct = true;
    d.status.is_timeout = false;
    d.status.is_time_jump_future = false;
    d.status.is_time_jump_past = false;

    d.sync_fup_data.precise_origin_timestamp = 100'000'000'000ULL;
    d.sync_fup_data.reference_global_timestamp = 100'000'000'500ULL;
    d.sync_fup_data.reference_local_timestamp = 100'000'001'000ULL;
    d.sync_fup_data.sync_ingress_timestamp = 100'000'001'000ULL;
    d.sync_fup_data.correction_field = 8U;
    d.sync_fup_data.sequence_id = 55;
    d.sync_fup_data.pdelay = 4'000U;
    d.sync_fup_data.port_number = 1;
    d.sync_fup_data.clock_identity = 0xCAFEBABEDEAD0001ULL;

    d.pdelay_data.request_origin_timestamp = 200'000'000'000ULL;
    d.pdelay_data.request_receipt_timestamp = 200'000'001'000ULL;
    d.pdelay_data.response_origin_timestamp = 200'000'001'000ULL;
    d.pdelay_data.response_receipt_timestamp = 200'000'002'000ULL;
    d.pdelay_data.pdelay = 1'000U;
    d.pdelay_data.req_port_number = 2;
    d.pdelay_data.resp_port_number = 3;
    d.pdelay_data.req_clock_identity = 0x0102030405060708ULL;
    d.pdelay_data.resp_clock_identity = 0x0807060504030201ULL;
    return d;
}

}  // namespace

class ShmPTPEngineTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        name_ = UniqueShmName();
        engine_ = std::make_unique<ShmPTPEngine>(name_);
    }

    void TearDown() override
    {
        engine_->Deinitialize();
        pub_.Close();
    }

    std::string name_;
    score::ts::details::GptpIpcPublisher pub_;
    std::unique_ptr<ShmPTPEngine> engine_;
};

// ── Lifecycle ────────────────────────────────────────────────────────────────

TEST_F(ShmPTPEngineTest, Initialize_WhenShmNotExist_ReturnsFalse)
{
    EXPECT_FALSE(engine_->Initialize());
}

TEST_F(ShmPTPEngineTest, Initialize_WhenShmExists_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Open(name_));
    EXPECT_TRUE(engine_->Initialize());
}

TEST_F(ShmPTPEngineTest, Initialize_CalledTwiceWhenInitialized_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(engine_->Initialize());
    EXPECT_TRUE(engine_->Initialize());  // idempotent
}

TEST_F(ShmPTPEngineTest, Deinitialize_WhenNotInitialized_ReturnsTrue)
{
    EXPECT_TRUE(engine_->Deinitialize());
}

TEST_F(ShmPTPEngineTest, Deinitialize_AfterInitialize_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(engine_->Initialize());
    EXPECT_TRUE(engine_->Deinitialize());
}

TEST_F(ShmPTPEngineTest, Deinitialize_CalledTwice_BothReturnTrue)
{
    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(engine_->Initialize());
    EXPECT_TRUE(engine_->Deinitialize());
    EXPECT_TRUE(engine_->Deinitialize());
}

TEST_F(ShmPTPEngineTest, ReInitialize_AfterDeinitialize_Succeeds)
{
    ASSERT_TRUE(pub_.Open(name_));
    ASSERT_TRUE(engine_->Initialize());
    ASSERT_TRUE(engine_->Deinitialize());
    EXPECT_TRUE(engine_->Initialize());
}

// ── ReadPTPSnapshot ───────────────────────────────────────────────────────────

TEST_F(ShmPTPEngineTest, ReadPTPSnapshot_WhenNotInitialized_ReturnsFalse)
{
    PtpTimeInfo info{};
    EXPECT_FALSE(engine_->ReadPTPSnapshot(info));
}

TEST_F(ShmPTPEngineTest, ReadPTPSnapshot_WithPublishedData_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Open(name_));
    pub_.Publish(MakeTestIpcData());
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    EXPECT_TRUE(engine_->ReadPTPSnapshot(result));
}

TEST_F(ShmPTPEngineTest, ReadPTPSnapshot_CopiesTimeAndStatusCorrectly)
{
    ASSERT_TRUE(pub_.Open(name_));
    const score::ts::GptpIpcData src = MakeTestIpcData();
    pub_.Publish(src);
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(result));

    EXPECT_EQ(result.ptp_assumed_time, src.ptp_assumed_time);
    EXPECT_DOUBLE_EQ(result.rate_deviation, src.rate_deviation);
    EXPECT_EQ(result.status.is_synchronized, src.status.is_synchronized);
    EXPECT_EQ(result.status.is_correct, src.status.is_correct);
    EXPECT_EQ(result.status.is_timeout, src.status.is_timeout);
}

TEST_F(ShmPTPEngineTest, ReadPTPSnapshot_CopiesSyncFupDataCorrectly)
{
    ASSERT_TRUE(pub_.Open(name_));
    const score::ts::GptpIpcData src = MakeTestIpcData();
    pub_.Publish(src);
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(result));

    EXPECT_EQ(result.sync_fup_data.precise_origin_timestamp, src.sync_fup_data.precise_origin_timestamp);
    EXPECT_EQ(result.sync_fup_data.reference_global_timestamp, src.sync_fup_data.reference_global_timestamp);
    EXPECT_EQ(result.sync_fup_data.sequence_id, src.sync_fup_data.sequence_id);
    EXPECT_EQ(result.sync_fup_data.pdelay, src.sync_fup_data.pdelay);
    EXPECT_EQ(result.sync_fup_data.clock_identity, src.sync_fup_data.clock_identity);
}

TEST_F(ShmPTPEngineTest, ReadPTPSnapshot_CopiesPDelayDataCorrectly)
{
    ASSERT_TRUE(pub_.Open(name_));
    const score::ts::GptpIpcData src = MakeTestIpcData();
    pub_.Publish(src);
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(result));

    EXPECT_EQ(result.pdelay_data.pdelay, src.pdelay_data.pdelay);
    EXPECT_EQ(result.pdelay_data.req_port_number, src.pdelay_data.req_port_number);
    EXPECT_EQ(result.pdelay_data.resp_port_number, src.pdelay_data.resp_port_number);
    EXPECT_EQ(result.pdelay_data.req_clock_identity, src.pdelay_data.req_clock_identity);
    EXPECT_EQ(result.pdelay_data.resp_clock_identity, src.pdelay_data.resp_clock_identity);
}

}  // namespace details
}  // namespace td
}  // namespace score
