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
#include "score/time_slave/src/gptp/details/pdelay_measurer.h"

#include <gtest/gtest.h>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

// Build a PTPMessage suitable for OnResponse / OnResponseFollowUp.
// seqId must be 0 to match the default-constructed req_ inside PeerDelayMeasurer
// (req_.ptpHdr.sequenceId == 0 before SendRequest is ever called).
PTPMessage MakeResp(std::uint16_t seqId,
                    std::int64_t parse_ts_ns,     // t2 or t3
                    std::int64_t recv_hw_ns = 0,  // t4 (only used in Resp, not FUP)
                    std::int64_t corr_ns = 0) noexcept
{
    PTPMessage msg{};
    msg.ptpHdr.sequenceId = seqId;
    msg.ptpHdr.correctionField = corr_ns << 16;  // CorrectionToTmv does >> 16
    msg.parseMessageTs.ns = parse_ts_ns;
    msg.recvHardwareTS.ns = recv_hw_ns;
    return msg;
}

}  // namespace

class PeerDelayMeasurerTest : public ::testing::Test
{
  protected:
    // ClockIdentity is all-zeros; sufficient for the delay computation tests.
    PeerDelayMeasurer measurer_{ClockIdentity{}};
};

// ── Default state ─────────────────────────────────────────────────────────────

TEST_F(PeerDelayMeasurerTest, GetResult_BeforeAnyMessage_IsInvalid)
{
    EXPECT_FALSE(measurer_.GetResult().valid);
    EXPECT_EQ(measurer_.GetResult().path_delay_ns, 0LL);
}

// ── Sequence-ID mismatch guards ───────────────────────────────────────────────

TEST_F(PeerDelayMeasurerTest, SeqIdMismatch_BetweenReqAndResp_NoResult)
{
    // Default req_.ptpHdr.sequenceId == 0; resp has seqId == 1 → mismatch.
    measurer_.OnResponse(MakeResp(1U, 100LL, 180LL));
    measurer_.OnResponseFollowUp(MakeResp(1U, 80LL));
    EXPECT_FALSE(measurer_.GetResult().valid);
}

TEST_F(PeerDelayMeasurerTest, SeqIdMismatch_BetweenRespAndFup_NoResult)
{
    // resp seqId == 0 (matches default req_), resp_fup seqId == 1 → mismatch.
    measurer_.OnResponse(MakeResp(0U, 100LL, 180LL));
    measurer_.OnResponseFollowUp(MakeResp(1U, 80LL));
    EXPECT_FALSE(measurer_.GetResult().valid);
}

// ── Delay computation (symmetric link) ───────────────────────────────────────
//
// Default req_ gives:  t1 = 0 ns (sendHardwareTS == 0)
//
// Chosen timestamps:
//   t2 (resp.parseMessageTs)    = 100 ns  (remote receipt time)
//   t3 (resp_fup.parseMessageTs) = 80 ns  (remote send time)
//   t4 (resp.recvHardwareTS)    = 180 ns  (local receive time)
//
// delay = ((t2 − t1) + (t4 − t3)) / 2
//       = ((100 − 0) + (180 − 80)) / 2
//       = (100 + 100) / 2
//       = 100 ns

TEST_F(PeerDelayMeasurerTest, Computation_SymmetricLink_CorrectDelay)
{
    measurer_.OnResponse(MakeResp(0U, /*t2=*/100LL, /*t4=*/180LL));
    measurer_.OnResponseFollowUp(MakeResp(0U, /*t3=*/80LL));

    const PDelayResult r = measurer_.GetResult();
    ASSERT_TRUE(r.valid);
    EXPECT_EQ(r.path_delay_ns, 100LL);
}

TEST_F(PeerDelayMeasurerTest, Computation_AsymmetricLink_CorrectDelay)
{
    // t1=0, t2=200, t3=150, t4=400  →  ((200-0) + (400-150)) / 2 = (200+250)/2 = 225
    measurer_.OnResponse(MakeResp(0U, 200LL, 400LL));
    measurer_.OnResponseFollowUp(MakeResp(0U, 150LL));

    const PDelayResult r = measurer_.GetResult();
    ASSERT_TRUE(r.valid);
    EXPECT_EQ(r.path_delay_ns, 225LL);
}

// ── Correction field applied to t3 ───────────────────────────────────────────
//
// t1=0, t2=100, t4=180
// t3=80 ns, correction_resp = 2 ns (stored as 2<<16), correction_fup = 0
// t3c = t3 + c1 + c2 = 80 + 2 + 0 = 82
// delay = ((100-0) + (180-82)) / 2 = (100+98) / 2 = 99

TEST_F(PeerDelayMeasurerTest, Computation_CorrectionField_AppliedToT3)
{
    measurer_.OnResponse(MakeResp(0U, 100LL, 180LL, /*corr_ns=*/2LL));
    measurer_.OnResponseFollowUp(MakeResp(0U, 80LL));

    const PDelayResult r = measurer_.GetResult();
    ASSERT_TRUE(r.valid);
    EXPECT_EQ(r.path_delay_ns, 99LL);
}

// ── PDelayData fields ─────────────────────────────────────────────────────────

TEST_F(PeerDelayMeasurerTest, PDelayData_TimestampFields_PopulatedCorrectly)
{
    measurer_.OnResponse(MakeResp(0U, /*t2=*/100LL, /*t4=*/180LL));
    measurer_.OnResponseFollowUp(MakeResp(0U, /*t3=*/80LL));

    const score::ts::GptpIpcPDelayData& d = measurer_.GetResult().pdelay_data;
    EXPECT_EQ(d.request_origin_timestamp, 0ULL);      // t1
    EXPECT_EQ(d.request_receipt_timestamp, 100ULL);   // t2
    EXPECT_EQ(d.response_origin_timestamp, 80ULL);    // t3
    EXPECT_EQ(d.response_receipt_timestamp, 180ULL);  // t4
    EXPECT_EQ(d.pdelay, 100ULL);                      // computed delay
}

// ── Multiple cycles: result updated on each valid completion ──────────────────

TEST_F(PeerDelayMeasurerTest, SecondCycle_OverwritesPreviousResult)
{
    // First measurement: delay = 100 ns
    measurer_.OnResponse(MakeResp(0U, 100LL, 180LL));
    measurer_.OnResponseFollowUp(MakeResp(0U, 80LL));
    ASSERT_TRUE(measurer_.GetResult().valid);

    // Second measurement with same seqId (still 0): delay = 50 ns
    // t1=0, t2=50, t3=25, t4=100  → ((50+75)/2=62 ... let me recalculate)
    // t1=0, t2=50, t4=100, t3=50  → ((50-0)+(100-50))/2 = (50+50)/2 = 50
    measurer_.OnResponse(MakeResp(0U, 50LL, 100LL));
    measurer_.OnResponseFollowUp(MakeResp(0U, 50LL));

    EXPECT_EQ(measurer_.GetResult().path_delay_ns, 50LL);
}

}  // namespace details
}  // namespace ts
}  // namespace score
