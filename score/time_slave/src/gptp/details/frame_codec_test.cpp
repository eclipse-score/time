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
#include "score/time_slave/src/gptp/details/frame_codec.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <array>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

// Build a minimal raw Ethernet frame with the given EtherType in the ethhdr.
// The buffer is zero-initialized; callers fill in anything extra.
std::vector<std::uint8_t> MakeEthFrame(std::uint16_t etype, int total_len)
{
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(total_len), 0);
    // h_proto at bytes 12-13 (big-endian)
    const std::uint16_t etype_be = htons(etype);
    std::memcpy(&buf[12], &etype_be, 2);
    return buf;
}

}  // namespace

class FrameCodecParseTest : public ::testing::Test
{
  protected:
    FrameCodec codec_;
};

// ── ParseEthernetHeader ───────────────────────────────────────────────────────

TEST_F(FrameCodecParseTest, TooShort_ReturnsFalse)
{
    std::uint8_t tiny[10] = {};
    int offset = -1;
    EXPECT_FALSE(codec_.ParseEthernetHeader(tiny, 10, offset));
}

TEST_F(FrameCodecParseTest, ExactlyEthHdrLength_NonPtp_ReturnsFalse)
{
    // 14 bytes, EtherType = 0x0800 (IPv4) — not PTP and not VLAN
    auto buf = MakeEthFrame(0x0800, 14);
    int offset = -1;
    EXPECT_FALSE(codec_.ParseEthernetHeader(buf.data(), 14, offset));
}

TEST_F(FrameCodecParseTest, Eth1588_Valid_ReturnsTrueAndOffset14)
{
    // Plain PTP frame: ethhdr(14) + PTP payload
    auto buf = MakeEthFrame(static_cast<std::uint16_t>(kEthP1588), 80);
    int offset = -1;
    ASSERT_TRUE(codec_.ParseEthernetHeader(buf.data(), 80, offset));
    EXPECT_EQ(offset, 14);  // PTP payload immediately after ethhdr
}

TEST_F(FrameCodecParseTest, Vlan8021Q_ValidPtpInner_ReturnsTrueAndOffset18)
{
    // IEEE 802.1Q layout: ethhdr(14) | TCI(2) | inner EtherType(2) | payload
    //   offset 14-15: TCI
    //   offset 16-17: inner EtherType  ← written here
    //   offset 18+  : PTP payload      ← ptp_offset == 18 == 14 + kVlanTagLen
    auto buf = MakeEthFrame(static_cast<std::uint16_t>(kEthP8021Q), 60);
    const std::uint16_t inner_be = htons(static_cast<std::uint16_t>(kEthP1588));
    std::memcpy(&buf[14 + 2], &inner_be, 2);  // inner EtherType at offset 16
    int offset = -1;
    ASSERT_TRUE(codec_.ParseEthernetHeader(buf.data(), 60, offset));
    EXPECT_EQ(offset, 14 + kVlanTagLen);  // PTP payload at offset 18
}

TEST_F(FrameCodecParseTest, Vlan8021Q_TooShortForInnerType_ReturnsFalse)
{
    // kEthHdrLen(14) + kVlanTagLen(4) + 2 = 20; provide only 19 bytes
    auto buf = MakeEthFrame(static_cast<std::uint16_t>(kEthP8021Q), 19);
    int offset = -1;
    EXPECT_FALSE(codec_.ParseEthernetHeader(buf.data(), 19, offset));
}

TEST_F(FrameCodecParseTest, Vlan8021Q_NonPtpInnerType_ReturnsFalse)
{
    auto buf = MakeEthFrame(static_cast<std::uint16_t>(kEthP8021Q), 30);
    // Inner EtherType = IPv4 (non-PTP)
    const std::uint16_t inner_be = htons(0x0800U);
    std::memcpy(&buf[14 + kVlanTagLen], &inner_be, 2);
    int offset = -1;
    EXPECT_FALSE(codec_.ParseEthernetHeader(buf.data(), 30, offset));
}

TEST_F(FrameCodecParseTest, UnknownEtherType_ReturnsFalse)
{
    auto buf = MakeEthFrame(0xABCDU, 60);
    int offset = -1;
    EXPECT_FALSE(codec_.ParseEthernetHeader(buf.data(), 60, offset));
}

// ── AddEthernetHeader ─────────────────────────────────────────────────────────

TEST_F(FrameCodecParseTest, AddEthernetHeader_NormalPayload_ReturnsTrueAndIncrementsLen)
{
    // Buffer large enough for payload + 14-byte header
    constexpr unsigned int kPayloadLen = 44U;
    std::uint8_t buf[256] = {};
    // Put a sentinel in the payload area so we can verify the shift
    buf[0] = 0xDE;
    buf[1] = 0xAD;

    unsigned int len = kPayloadLen;
    const std::array<std::uint8_t, kMacAddrLen> src_mac = {0x02U, 0x00U, 0x00U, 0xFFU, 0x00U, 0x11U};
    ASSERT_TRUE(codec_.AddEthernetHeader(buf, len, src_mac, sizeof(buf)));
    EXPECT_EQ(len, kPayloadLen + 14U);

    // Payload was shifted right by 14 bytes
    EXPECT_EQ(buf[14], 0xDE);
    EXPECT_EQ(buf[15], 0xAD);

    // h_proto at bytes 12-13 should be kEthP1588 in network byte order
    const std::uint16_t h_proto_be = htons(static_cast<std::uint16_t>(kEthP1588));
    std::uint16_t actual{};
    std::memcpy(&actual, &buf[12], 2);
    EXPECT_EQ(actual, h_proto_be);
}

TEST_F(FrameCodecParseTest, AddEthernetHeader_PayloadTooLarge_ReturnsFalse)
{
    constexpr unsigned int kTooBig = 2048U;  // buf_len + 14 > capacity
    std::uint8_t buf[4096] = {};
    unsigned int len = kTooBig;
    const std::array<std::uint8_t, kMacAddrLen> src_mac = {0x02U, 0x00U, 0x00U, 0xFFU, 0x00U, 0x11U};
    EXPECT_FALSE(codec_.AddEthernetHeader(buf, len, src_mac, kTooBig));
}

}  // namespace details
}  // namespace ts
}  // namespace score
