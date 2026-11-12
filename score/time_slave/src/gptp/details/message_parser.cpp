/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <arpa/inet.h>
#include <cstring>

namespace
{

inline std::uint64_t ByteSwap64(std::uint64_t v) noexcept
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(v);
#else
    return v;
#endif
}

}  // namespace

namespace score
{
namespace ts
{
namespace details
{

namespace
{

std::uint16_t LoadU16(const std::uint8_t* p) noexcept
{
    std::uint16_t v{};
    std::memcpy(&v, p, sizeof(v));
    return ntohs(v);
}

std::uint32_t LoadU32(const std::uint8_t* p) noexcept
{
    std::uint32_t v{};
    std::memcpy(&v, p, sizeof(v));
    return ntohl(v);
}

std::uint64_t LoadBe64(const std::uint8_t* p) noexcept
{
    std::uint64_t v{};
    std::memcpy(&v, p, sizeof(v));
    return ByteSwap64(v);
}

Timestamp LoadTimestamp(const std::uint8_t* p) noexcept
{
    Timestamp ts{};
    ts.seconds_msb = LoadU16(p);
    ts.seconds_lsb = LoadU32(p + 2);
    ts.nanoseconds = LoadU32(p + 6);
    return ts;
}

PortIdentity LoadPortIdentity(const std::uint8_t* p) noexcept
{
    PortIdentity pid{};
    std::memcpy(pid.clockIdentity.id, p, 8);
    pid.portNumber = LoadU16(p + 8);
    return pid;
}

}  // namespace

bool GptpMessageParser::Parse(const std::uint8_t* payload, std::size_t payload_len, PTPMessage& msg) const
{
    if (payload == nullptr || payload_len < sizeof(PTPHeader))
        return false;

    const std::uint16_t declared_len =
        static_cast<std::uint16_t>((static_cast<std::uint16_t>(payload[2]) << 8U) | payload[3]);
    if (declared_len < sizeof(PTPHeader) || static_cast<std::size_t>(declared_len) > payload_len)
        return false;

    // Validate transportSpecific nibble (must be 0x1 for 802.1AS) and PTP version.
    if ((payload[0] & 0xF0U) != kPtpTransportSpecific)
        return false;
    if ((payload[1] & 0x0FU) != kPtpVersion)
        return false;

    msg.ptpHdr.tsmt = payload[0];
    msg.ptpHdr.version = payload[1];
    msg.ptpHdr.messageLength = LoadU16(payload + 2);
    msg.ptpHdr.domainNumber = payload[4];
    msg.ptpHdr.reserved1 = payload[5];
    std::memcpy(msg.ptpHdr.flagField, payload + 6, 2);
    msg.ptpHdr.correctionField = static_cast<std::int64_t>(LoadBe64(payload + 8));
    msg.ptpHdr.reserved2 = LoadU32(payload + 16);
    std::memcpy(msg.ptpHdr.sourcePortIdentity.clockIdentity.id, payload + 20, 8);
    msg.ptpHdr.sourcePortIdentity.portNumber = LoadU16(payload + 28);
    msg.ptpHdr.sequenceId = LoadU16(payload + 30);
    msg.ptpHdr.controlField = payload[32];
    msg.ptpHdr.logMessageInterval = static_cast<std::int8_t>(payload[33]);

    msg.msgtype = msg.ptpHdr.tsmt & 0x0FU;

    constexpr std::size_t kBodyOffset = 34U;
    constexpr std::size_t kPidSize = 10U;  // ClockIdentity (8) + portNumber (2)
    // Pdelay_Req body: originTimestamp (10 bytes, sent as zero, ignored by receivers)
    // followed by 10 reserved bytes. Neither field is parsed; only its presence is validated.
    constexpr std::size_t kPdelayReqBodySize = 20U;
    constexpr std::size_t kTimestampPlusPidSize = sizeof(Timestamp) + kPidSize;

    // Each case first rejects a declared length shorter than the body it is about to read
    // (IEEE 802.1AS), so truncated messages are never delivered with zero-initialised fields.
    const std::size_t declared = static_cast<std::size_t>(declared_len);
    switch (msg.msgtype)
    {
        case kPtpMsgtypeSync:
            if (declared < kBodyOffset + sizeof(Timestamp))
                return false;
            break;

        case kPtpMsgtypeFollowUp:
            if (declared < kBodyOffset + sizeof(Timestamp))
                return false;
            msg.follow_up.preciseOriginTimestamp = LoadTimestamp(payload + kBodyOffset);
            break;

        case kPtpMsgtypePdelayReq:
            if (declared < kBodyOffset + kPdelayReqBodySize)
                return false;
            break;

        case kPtpMsgtypePdelayResp:
            if (declared < kBodyOffset + kTimestampPlusPidSize)
                return false;
            msg.pdelay_resp.requestReceiptTimestamp = LoadTimestamp(payload + kBodyOffset);
            // requestingPortIdentity lets ComputeAndStoreUnlocked() verify the response is addressed to us.
            msg.pdelay_resp.requestingPortIdentity = LoadPortIdentity(payload + kBodyOffset + sizeof(Timestamp));
            break;

        case kPtpMsgtypePdelayRespFollowUp:
            if (declared < kBodyOffset + kTimestampPlusPidSize)
                return false;
            msg.pdelay_resp_fup.responseOriginReceiptTimestamp = LoadTimestamp(payload + kBodyOffset);
            msg.pdelay_resp_fup.requestingPortIdentity = LoadPortIdentity(payload + kBodyOffset + sizeof(Timestamp));
            break;

        default:
            break;
    }

    return true;
}

}  // namespace details
}  // namespace ts
}  // namespace score
