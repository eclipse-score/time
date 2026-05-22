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
#include "score/time_slave/src/gptp/details/sync_state_machine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

// Build a Sync PTPMessage with the given sequence ID and hardware RX timestamp.
// The correctionField encodes correction in sub-ns units (<<16 so >>16 == 0).
PTPMessage MakeSync(std::uint16_t seqId, std::int64_t recv_hw_ns, std::int64_t corr_ns = 0LL) noexcept
{
    PTPMessage msg{};
    msg.msgtype = kPtpMsgtypeSync;
    msg.ptpHdr.sequenceId = seqId;
    msg.ptpHdr.correctionField = corr_ns << 16;  // CorrectionToTmv does >> 16
    msg.recvHardwareTS.ns = recv_hw_ns;
    return msg;
}

// Build a FollowUp PTPMessage with the given sequence ID and precise origin
// timestamp (in nanoseconds since epoch).
PTPMessage MakeFollowUp(std::uint16_t seqId, std::int64_t origin_ns, std::int64_t corr_ns = 0LL) noexcept
{
    PTPMessage msg{};
    msg.msgtype = kPtpMsgtypeFollowUp;
    msg.ptpHdr.sequenceId = seqId;
    msg.ptpHdr.correctionField = corr_ns << 16;
    // Encode origin_ns into the preciseOriginTimestamp wire field.
    msg.follow_up.preciseOriginTimestamp = TmvToTimestamp(TmvT{origin_ns});
    return msg;
}

// Helper: deliver a matching Sync+FollowUp pair and return the SyncResult.
// Aborts the test if the pair does not produce a result.
SyncResult DeliverPair(SyncStateMachine& ssm, std::uint16_t seqId, std::int64_t recv_hw_ns, std::int64_t origin_ns)
{
    ssm.OnSync(MakeSync(seqId, recv_hw_ns));
    auto result = ssm.OnFollowUp(MakeFollowUp(seqId, origin_ns));
    if (!result.has_value())
        ADD_FAILURE() << "Expected SyncResult but got nullopt";
    return result.value_or(SyncResult{});
}

}  // namespace

class SyncStateMachineTest : public ::testing::Test
{
  protected:
    // threshold = 500 ms
    SyncStateMachine ssm_{500'000'000LL};
};

// ── Basic pairing ─────────────────────────────────────────────────────────────

TEST_F(SyncStateMachineTest, SyncThenFollowUp_MatchingSeq_ReturnsSyncResult)
{
    ssm_.OnSync(MakeSync(1U, 1'000'000'000LL));
    auto result = ssm_.OnFollowUp(MakeFollowUp(1U, 900'000'000LL));
    ASSERT_TRUE(result.has_value());
    // master_ns = origin_ns (no correction)
    EXPECT_EQ(result->master_ns, 900'000'000LL);
    // offset = recv_hw - master
    EXPECT_EQ(result->offset_ns, 1'000'000'000LL - 900'000'000LL);
}

TEST_F(SyncStateMachineTest, FollowUpBeforeSync_ReturnsNullopt)
{
    // kEmpty state: FUP arrives first → buffered, no result yet
    auto result = ssm_.OnFollowUp(MakeFollowUp(1U, 0LL));
    EXPECT_FALSE(result.has_value());
}

TEST_F(SyncStateMachineTest, MultipleSyncs_ThenFollowUp_UsesLatestSync)
{
    // Two Syncs without a FUP between them — newer Sync should be used
    ssm_.OnSync(MakeSync(1U, 1'000'000'000LL));
    ssm_.OnSync(MakeSync(2U, 2'000'000'000LL));
    // FUP with seqId == 2 (matches the newer Sync)
    auto result = ssm_.OnFollowUp(MakeFollowUp(2U, 1'800'000'000LL));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->master_ns, 1'800'000'000LL);
}

TEST_F(SyncStateMachineTest, SeqIdMismatch_ReturnsNullopt_ThenMatchesOnNext)
{
    ssm_.OnSync(MakeSync(10U, 1'000'000'000LL));
    // FUP for a different seqId → no result; state becomes kHaveFup
    auto r1 = ssm_.OnFollowUp(MakeFollowUp(99U, 0LL));
    EXPECT_FALSE(r1.has_value());

    // Now deliver a Sync that matches the buffered FUP
    ssm_.OnSync(MakeSync(99U, 2'000'000'000LL));
    auto r2 = ssm_.OnFollowUp(MakeFollowUp(99U, 1'900'000'000LL));
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->master_ns, 1'900'000'000LL);
}

// ── SyncFupData fields ────────────────────────────────────────────────────────

TEST_F(SyncStateMachineTest, SyncFupData_SequenceId_SetFromFollowUp)
{
    const std::uint16_t kSeq = 42U;
    ssm_.OnSync(MakeSync(kSeq, 1'000'000'000LL));
    auto result = ssm_.OnFollowUp(MakeFollowUp(kSeq, 900'000'000LL));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->sync_fup_data.sequence_id, kSeq);
}

TEST_F(SyncStateMachineTest, SyncFupData_PreciseOriginTimestamp_MatchesInput)
{
    const std::int64_t kOrigin = 5'000'000'000LL;  // 5 s
    ssm_.OnSync(MakeSync(1U, 6'000'000'000LL));
    auto result = ssm_.OnFollowUp(MakeFollowUp(1U, kOrigin));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(static_cast<std::int64_t>(result->sync_fup_data.precise_origin_timestamp), kOrigin);
}

// ── Jump detection ────────────────────────────────────────────────────────────

TEST_F(SyncStateMachineTest, JumpPast_Detected_OnSecondPair)
{
    // First pair establishes baseline master_ns = 2 s
    DeliverPair(ssm_, 1U, 2'100'000'000LL, 2'000'000'000LL);

    // Second pair: master_ns goes backward → is_time_jump_past
    std::ignore = ssm_.OnFollowUp(MakeFollowUp(2U, 1'000'000'000LL));  // no Sync preceding this on new seqId

    ssm_.OnSync(MakeSync(2U, 3'000'000'000LL));
    auto r2 = ssm_.OnFollowUp(MakeFollowUp(2U, 1'000'000'000LL));
    ASSERT_TRUE(r2.has_value());
    EXPECT_TRUE(r2->is_time_jump_past);
    EXPECT_FALSE(r2->is_time_jump_future);
}

TEST_F(SyncStateMachineTest, JumpFuture_Detected_WhenDeltaExceedsThreshold)
{
    // First pair: master_ns = 1 s
    DeliverPair(ssm_, 1U, 1'100'000'000LL, 1'000'000'000LL);

    // Second pair: master_ns jumps by 2 s > threshold (500 ms)
    ssm_.OnSync(MakeSync(2U, 3'100'000'000LL));
    auto r2 = ssm_.OnFollowUp(MakeFollowUp(2U, 3'000'000'000LL));
    ASSERT_TRUE(r2.has_value());
    EXPECT_TRUE(r2->is_time_jump_future);
    EXPECT_FALSE(r2->is_time_jump_past);
}

TEST_F(SyncStateMachineTest, NoJump_WhenFirstPair)
{
    // First pair — no previous baseline; no jump should be flagged
    ssm_.OnSync(MakeSync(1U, 1'000'000'000LL));
    auto result = ssm_.OnFollowUp(MakeFollowUp(1U, 900'000'000LL));
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->is_time_jump_past);
    EXPECT_FALSE(result->is_time_jump_future);
}

// ── neighborRateRatio ─────────────────────────────────────────────────────────

TEST_F(SyncStateMachineTest, NeighborRateRatio_Default_IsOne)
{
    EXPECT_DOUBLE_EQ(ssm_.GetNeighborRateRatio(), 1.0);
}

TEST_F(SyncStateMachineTest, NeighborRateRatio_AfterTwoPairs_Computed)
{
    // Pair 1: slave_rx = 1000 ms, master_origin = 1000 ms
    DeliverPair(ssm_, 1U, 1'000'000'000LL, 1'000'000'000LL);

    // Pair 2: slave_rx = 2000 ms (+1000 ms), master_origin = 2010 ms (+1010 ms)
    // ratio = 1000_000_000 / 1010_000_000 ≈ 0.99009...
    DeliverPair(ssm_, 2U, 2'000'000'000LL, 2'010'000'000LL);

    const double expected = 1'000'000'000.0 / 1'010'000'000.0;
    EXPECT_NEAR(ssm_.GetNeighborRateRatio(), expected, 1e-9);
}

// ── IsTimeout ─────────────────────────────────────────────────────────────────

TEST_F(SyncStateMachineTest, IsTimeout_BeforeFirstSync_ReturnsFalse)
{
    // Before first sync, IsTimeout uses the object creation time as baseline.
    // Passing now=0 gives (0 - created_mono_ns_) which is negative → not > threshold.
    EXPECT_FALSE(ssm_.IsTimeout(0LL, 1LL));
}

TEST_F(SyncStateMachineTest, IsTimeout_AfterSuccessfulPair_WithLargeNow_ReturnsTrue)
{
    DeliverPair(ssm_, 1U, 1'000'000'000LL, 900'000'000LL);
    // Provide a mono_now far in the future; timeout = 1 s
    EXPECT_TRUE(ssm_.IsTimeout(std::numeric_limits<std::int64_t>::max(), 1'000'000'000LL));
}

TEST_F(SyncStateMachineTest, IsTimeout_AfterSuccessfulPair_WithSmallDelta_ReturnsFalse)
{
    DeliverPair(ssm_, 1U, 1'000'000'000LL, 900'000'000LL);
    // Provide mono_now = 0, which is before the recorded timestamp → not timed out
    EXPECT_FALSE(ssm_.IsTimeout(0LL, 1'000'000'000LL));
}

TEST_F(SyncStateMachineTest, IsTimeout_ZeroTimeout_AlwaysReturnsFalse)
{
    DeliverPair(ssm_, 1U, 1'000'000'000LL, 900'000'000LL);
    EXPECT_FALSE(ssm_.IsTimeout(std::numeric_limits<std::int64_t>::max(), 0LL));
}

}  // namespace details
}  // namespace ts
}  // namespace score
