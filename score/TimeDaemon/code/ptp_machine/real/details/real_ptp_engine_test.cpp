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
#include "score/TimeDaemon/code/ptp_machine/real/details/real_ptp_engine.h"

#include "score/libTSClient/gptp_ipc_publisher.h"

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
    return "/gptp_rpe_ut_" + std::to_string(::getpid()) + "_" +
           std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

// Build a fully-populated PtpTimeInfo for roundtrip verification.
PtpTimeInfo MakeTestInfo()
{
    PtpTimeInfo info{};
    info.ptp_assumed_time = std::chrono::nanoseconds{9'876'543'210LL};
    info.rate_deviation   = -0.25;

    info.status.is_synchronized     = true;
    info.status.is_correct          = true;
    info.status.is_timeout          = false;
    info.status.is_time_jump_future = false;
    info.status.is_time_jump_past   = false;

    info.sync_fup_data.precise_origin_timestamp   = 100'000'000'000ULL;
    info.sync_fup_data.reference_global_timestamp = 100'000'000'500ULL;
    info.sync_fup_data.reference_local_timestamp  = 100'000'001'000ULL;
    info.sync_fup_data.sync_ingress_timestamp     = 100'000'001'000ULL;
    info.sync_fup_data.correction_field           = 8U;
    info.sync_fup_data.sequence_id                = 55;
    info.sync_fup_data.pdelay                     = 4'000U;
    info.sync_fup_data.port_number                = 1;
    info.sync_fup_data.clock_identity             = 0xCAFEBABEDEAD0001ULL;

    info.pdelay_data.request_origin_timestamp   = 200'000'000'000ULL;
    info.pdelay_data.request_receipt_timestamp  = 200'000'001'000ULL;
    info.pdelay_data.response_origin_timestamp  = 200'000'001'000ULL;
    info.pdelay_data.response_receipt_timestamp = 200'000'002'000ULL;
    info.pdelay_data.pdelay                     = 1'000U;
    info.pdelay_data.req_port_number            = 2;
    info.pdelay_data.resp_port_number           = 3;
    info.pdelay_data.req_clock_identity         = 0x0102030405060708ULL;
    info.pdelay_data.resp_clock_identity        = 0x0807060504030201ULL;
    return info;
}

}  // namespace

class RealPTPEngineTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        name_   = UniqueShmName();
        engine_ = std::make_unique<RealPTPEngine>(name_);
    }

    void TearDown() override
    {
        engine_->Deinitialize();
        pub_.Destroy();
    }

    std::string                   name_;
    score::ts::details::GptpIpcPublisher pub_;
    std::unique_ptr<RealPTPEngine> engine_;
};

// ── Lifecycle ────────────────────────────────────────────────────────────────

TEST_F(RealPTPEngineTest, Initialize_WhenShmNotExist_ReturnsFalse)
{
    // No publisher → shm doesn't exist.
    EXPECT_FALSE(engine_->Initialize());
}

TEST_F(RealPTPEngineTest, Initialize_WhenShmExists_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    EXPECT_TRUE(engine_->Initialize());
}

TEST_F(RealPTPEngineTest, Initialize_CalledTwiceWhenInitialized_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(engine_->Initialize());
    EXPECT_TRUE(engine_->Initialize());  // idempotent
}

TEST_F(RealPTPEngineTest, Deinitialize_WhenNotInitialized_ReturnsTrue)
{
    EXPECT_TRUE(engine_->Deinitialize());
}

TEST_F(RealPTPEngineTest, Deinitialize_AfterInitialize_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(engine_->Initialize());
    EXPECT_TRUE(engine_->Deinitialize());
}

TEST_F(RealPTPEngineTest, Deinitialize_CalledTwice_BothReturnTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(engine_->Initialize());
    EXPECT_TRUE(engine_->Deinitialize());
    EXPECT_TRUE(engine_->Deinitialize());
}

TEST_F(RealPTPEngineTest, ReInitialize_AfterDeinitialize_Succeeds)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(engine_->Initialize());
    ASSERT_TRUE(engine_->Deinitialize());
    EXPECT_TRUE(engine_->Initialize());
}

// ── ReadPTPSnapshot ───────────────────────────────────────────────────────────

TEST_F(RealPTPEngineTest, ReadPTPSnapshot_WhenNotInitialized_ReturnsFalse)
{
    PtpTimeInfo info{};
    EXPECT_FALSE(engine_->ReadPTPSnapshot(info));
}

TEST_F(RealPTPEngineTest, ReadPTPSnapshot_WithPublishedData_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    pub_.Publish(MakeTestInfo());
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    EXPECT_TRUE(engine_->ReadPTPSnapshot(result));
}

TEST_F(RealPTPEngineTest, ReadPTPSnapshot_CopiesTimeAndStatusCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    const PtpTimeInfo expected = MakeTestInfo();
    pub_.Publish(expected);
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(result));

    EXPECT_EQ(result.ptp_assumed_time, expected.ptp_assumed_time);
    EXPECT_DOUBLE_EQ(result.rate_deviation, expected.rate_deviation);
    EXPECT_EQ(result.status.is_synchronized, expected.status.is_synchronized);
    EXPECT_EQ(result.status.is_correct, expected.status.is_correct);
    EXPECT_EQ(result.status.is_timeout, expected.status.is_timeout);
}

TEST_F(RealPTPEngineTest, ReadPTPSnapshot_CopiesSyncFupDataCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    const PtpTimeInfo expected = MakeTestInfo();
    pub_.Publish(expected);
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(result));

    EXPECT_EQ(result.sync_fup_data.precise_origin_timestamp,
              expected.sync_fup_data.precise_origin_timestamp);
    EXPECT_EQ(result.sync_fup_data.reference_global_timestamp,
              expected.sync_fup_data.reference_global_timestamp);
    EXPECT_EQ(result.sync_fup_data.sequence_id, expected.sync_fup_data.sequence_id);
    EXPECT_EQ(result.sync_fup_data.pdelay, expected.sync_fup_data.pdelay);
    EXPECT_EQ(result.sync_fup_data.clock_identity, expected.sync_fup_data.clock_identity);
}

TEST_F(RealPTPEngineTest, ReadPTPSnapshot_CopiesPDelayDataCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    const PtpTimeInfo expected = MakeTestInfo();
    pub_.Publish(expected);
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(result));

    EXPECT_EQ(result.pdelay_data.pdelay, expected.pdelay_data.pdelay);
    EXPECT_EQ(result.pdelay_data.req_port_number, expected.pdelay_data.req_port_number);
    EXPECT_EQ(result.pdelay_data.resp_port_number, expected.pdelay_data.resp_port_number);
    EXPECT_EQ(result.pdelay_data.req_clock_identity,
              expected.pdelay_data.req_clock_identity);
    EXPECT_EQ(result.pdelay_data.resp_clock_identity,
              expected.pdelay_data.resp_clock_identity);
}

// ── Individual sub-methods (called after ReadPTPSnapshot populates cache) ─────

TEST_F(RealPTPEngineTest, ReadTimeValueAndStatus_FromCachedData_AlwaysReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    pub_.Publish(MakeTestInfo());
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo snap{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(snap));

    // Call again on a fresh struct — should use the cached data.
    PtpTimeInfo result{};
    EXPECT_TRUE(engine_->ReadTimeValueAndStatus(result));
    EXPECT_EQ(result.ptp_assumed_time, snap.ptp_assumed_time);
    EXPECT_DOUBLE_EQ(result.rate_deviation, snap.rate_deviation);
    EXPECT_EQ(result.status.is_synchronized, snap.status.is_synchronized);
}

TEST_F(RealPTPEngineTest, ReadPDelayMeasurementData_FromCachedData_AlwaysReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    pub_.Publish(MakeTestInfo());
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo snap{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(snap));

    PtpTimeInfo result{};
    EXPECT_TRUE(engine_->ReadPDelayMeasurementData(result));
    EXPECT_EQ(result.pdelay_data.pdelay, snap.pdelay_data.pdelay);
}

TEST_F(RealPTPEngineTest, ReadSyncMeasurementData_FromCachedData_AlwaysReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    pub_.Publish(MakeTestInfo());
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo snap{};
    ASSERT_TRUE(engine_->ReadPTPSnapshot(snap));

    PtpTimeInfo result{};
    EXPECT_TRUE(engine_->ReadSyncMeasurementData(result));
    EXPECT_EQ(result.sync_fup_data.sequence_id, snap.sync_fup_data.sequence_id);
}

// Sub-methods on default-constructed cache (before any snapshot) return true
// with zeroed data.
TEST_F(RealPTPEngineTest, SubMethods_BeforeSnapshot_ReturnTrueWithZeroData)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(engine_->Initialize());

    PtpTimeInfo result{};
    EXPECT_TRUE(engine_->ReadTimeValueAndStatus(result));
    EXPECT_TRUE(engine_->ReadPDelayMeasurementData(result));
    EXPECT_TRUE(engine_->ReadSyncMeasurementData(result));
    EXPECT_EQ(result.ptp_assumed_time, std::chrono::nanoseconds{0});
}

}  // namespace details
}  // namespace td
}  // namespace score
