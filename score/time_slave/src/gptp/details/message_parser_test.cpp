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
#include "score/time_slave/src/gptp/details/message_parser.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <cstring>
#include <vector>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

// PTP header occupies exactly 34 bytes on the wire.
constexpr std::size_t kHdrSize = 34U;
// Timestamp body = 10 bytes (u16 + u32 + u32).
constexpr std::size_t kTsSize = 10U;

// Store a 16-bit big-endian value at buf[off].
void PutU16Be(std::uint8_t* buf, std::size_t off, std::uint16_t val)
{
    const std::uint16_t v = htons(val);
    std::memcpy(buf + off, &v, 2);
}

// Store a 32-bit big-endian value at buf[off].
void PutU32Be(std::uint8_t* buf, std::size_t off, std::uint32_t val)
{
    const std::uint32_t v = htonl(val);
    std::memcpy(buf + off, &v, 4);
}

// Store a 64-bit big-endian value at buf[off].
void PutU64Be(std::uint8_t* buf, std::size_t off, std::uint64_t val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    val = __builtin_bswap64(val);
#endif
    std::memcpy(buf + off, &val, 8);
}

// Build a minimal PTP payload of type `msgtype` with the given header fields.
// Optionally appends a 10-byte Timestamp body (seconds_lsb + nanoseconds).
std::vector<std::uint8_t> BuildPayload(std::uint8_t msgtype,
                                       std::uint16_t seqId,
                                       std::int64_t correction = 0,
                                       std::uint16_t port_number = 0,
                                       std::uint64_t clock_id = 0,
                                       std::uint32_t ts_sec_lsb = 0,
                                       std::uint32_t ts_ns = 0)
{
    const std::size_t total = kHdrSize + kTsSize;
    std::vector<std::uint8_t> buf(total, 0);

    buf[0] = static_cast<std::uint8_t>((kPtpTransportSpecific) | (msgtype & 0x0FU));
    buf[1] = kPtpVersion;
    PutU16Be(buf.data(), 2, static_cast<std::uint16_t>(total));  // messageLength
    // domainNumber = 0 (default)
    PutU64Be(buf.data(), 8, static_cast<std::uint64_t>(correction));  // correctionField
    // Clock identity is a raw byte array; store in native order so ClockIdentityToU64 roundtrips.
    std::memcpy(buf.data() + 20, &clock_id, 8);
    PutU16Be(buf.data(), 28, port_number);
    PutU16Be(buf.data(), 30, seqId);
    buf[32] = static_cast<std::uint8_t>(ControlField::kFollowUp);

    // Timestamp body at offset 34: seconds_msb(u16) + seconds_lsb(u32) + nanoseconds(u32)
    PutU16Be(buf.data(), kHdrSize, 0U);  // seconds_msb = 0
    PutU32Be(buf.data(), kHdrSize + 2, ts_sec_lsb);
    PutU32Be(buf.data(), kHdrSize + 6, ts_ns);

    return buf;
}

}  // namespace

class MessageParserTest : public ::testing::Test
{
  protected:
    GptpMessageParser parser_;
};

// ── Rejection cases ───────────────────────────────────────────────────────────

TEST_F(MessageParserTest, NullPayload_ReturnsFalse)
{
    PTPMessage msg{};
    EXPECT_FALSE(parser_.Parse(nullptr, 64U, msg));
}

TEST_F(MessageParserTest, TooShortPayload_ReturnsFalse)
{
    std::uint8_t tiny[10] = {};
    PTPMessage msg{};
    EXPECT_FALSE(parser_.Parse(tiny, 10U, msg));
}

// ── Sync (no body decoded, only header) ───────────────────────────────────────

TEST_F(MessageParserTest, SyncMessage_ReturnsTrue_MsgtypeIsSync)
{
    auto buf = BuildPayload(kPtpMsgtypeSync, 7U);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.msgtype, kPtpMsgtypeSync);
}

TEST_F(MessageParserTest, Header_SequenceId_DecodedCorrectly)
{
    const std::uint16_t kSeq = 0x1234U;
    auto buf = BuildPayload(kPtpMsgtypeSync, kSeq);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.ptpHdr.sequenceId, kSeq);
}

TEST_F(MessageParserTest, Header_CorrectionField_DecodedCorrectly)
{
    // correctionField = 65536 (0x10000) → CorrectionToTmv would give 1 ns
    const std::int64_t kCorr = 65536LL;
    auto buf = BuildPayload(kPtpMsgtypeSync, 1U, kCorr);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.ptpHdr.correctionField, kCorr);
}

TEST_F(MessageParserTest, Header_SourcePortIdentity_DecodedCorrectly)
{
    const std::uint64_t kClockId = 0xCAFEBABEDEAD0001ULL;
    const std::uint16_t kPort = 3U;
    auto buf = BuildPayload(kPtpMsgtypeSync, 1U, 0, kPort, kClockId);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.ptpHdr.sourcePortIdentity.portNumber, kPort);
    EXPECT_EQ(ClockIdentityToU64(msg.ptpHdr.sourcePortIdentity.clockIdentity), kClockId);
}

// ── FollowUp body ─────────────────────────────────────────────────────────────

TEST_F(MessageParserTest, FollowUp_Body_TimestampDecodedCorrectly)
{
    // precise_origin = 2 seconds + 500_000_000 ns
    const std::uint32_t kSecLsb = 2U;
    const std::uint32_t kNs = 500'000'000U;
    auto buf = BuildPayload(kPtpMsgtypeFollowUp, 99U, 0, 0, 0, kSecLsb, kNs);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.msgtype, kPtpMsgtypeFollowUp);
    EXPECT_EQ(msg.follow_up.preciseOriginTimestamp.seconds_lsb, kSecLsb);
    EXPECT_EQ(msg.follow_up.preciseOriginTimestamp.nanoseconds, kNs);
}

// ── PdelayResp body ───────────────────────────────────────────────────────────

TEST_F(MessageParserTest, PdelayResp_Body_TimestampDecodedCorrectly)
{
    const std::uint32_t kSecLsb = 3U;
    const std::uint32_t kNs = 123'456'789U;
    auto buf = BuildPayload(kPtpMsgtypePdelayResp, 5U, 0, 0, 0, kSecLsb, kNs);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.msgtype, kPtpMsgtypePdelayResp);
    EXPECT_EQ(msg.pdelay_resp.requestReceiptTimestamp.seconds_lsb, kSecLsb);
    EXPECT_EQ(msg.pdelay_resp.requestReceiptTimestamp.nanoseconds, kNs);
}

// ── PdelayRespFollowUp body ───────────────────────────────────────────────────

TEST_F(MessageParserTest, PdelayRespFollowUp_Body_TimestampDecodedCorrectly)
{
    const std::uint32_t kSecLsb = 7U;
    const std::uint32_t kNs = 999'000'000U;
    auto buf = BuildPayload(kPtpMsgtypePdelayRespFollowUp, 11U, 0, 0, 0, kSecLsb, kNs);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.msgtype, kPtpMsgtypePdelayRespFollowUp);
    EXPECT_EQ(msg.pdelay_resp_fup.responseOriginReceiptTimestamp.seconds_lsb, kSecLsb);
    EXPECT_EQ(msg.pdelay_resp_fup.responseOriginReceiptTimestamp.nanoseconds, kNs);
}

// ── Unknown type: header parsed, no body crash ────────────────────────────────

TEST_F(MessageParserTest, UnknownMsgtype_ReturnsTrue_HeaderParsed)
{
    // Use PdelayReq (type 0x2) which has no special body decoding branch.
    auto buf = BuildPayload(kPtpMsgtypePdelayReq, 20U);
    PTPMessage msg{};
    ASSERT_TRUE(parser_.Parse(buf.data(), buf.size(), msg));
    EXPECT_EQ(msg.msgtype, kPtpMsgtypePdelayReq);
}

// ── TimestampToTmv / TmvToTimestamp overflow guards ───────────────────────────

TEST_F(MessageParserTest, TimestampToTmv_SecExceedsMax_ReturnsZero)
{
    // seconds_msb=3 → sec = 3 * 2^32 = 12,884,901,888 > kMaxSec (9,223,372,036)
    Timestamp ts{};
    ts.seconds_msb = 3U;
    ts.seconds_lsb = 0U;
    ts.nanoseconds = 0U;
    const TmvT result = TimestampToTmv(ts);
    EXPECT_EQ(result.ns, 0LL);
}

TEST_F(MessageParserTest, TimestampToTmv_TotalNsExceedsMax_ReturnsZero)
{
    // sec = kMaxSec = 9,223,372,036 (seconds_msb=2, seconds_lsb=633,437,444)
    // total_ns = kMaxSec * 1e9 + 854,775,808 > INT64_MAX
    Timestamp ts{};
    ts.seconds_msb = 2U;
    ts.seconds_lsb = 633'437'444U;
    ts.nanoseconds = 854'775'808U;
    const TmvT result = TimestampToTmv(ts);
    EXPECT_EQ(result.ns, 0LL);
}

TEST_F(MessageParserTest, TmvToTimestamp_NegativeNs_ReturnsZeroTimestamp)
{
    const Timestamp ts = TmvToTimestamp(TmvT{-1LL});
    EXPECT_EQ(ts.seconds_msb, 0U);
    EXPECT_EQ(ts.seconds_lsb, 0U);
    EXPECT_EQ(ts.nanoseconds, 0U);
}

}  // namespace details
}  // namespace ts
}  // namespace score
