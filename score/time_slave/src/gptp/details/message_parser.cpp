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

    switch (msg.msgtype)
    {
        case kPtpMsgtypeFollowUp:
            if (payload_len >= kBodyOffset + sizeof(Timestamp))
                msg.follow_up.preciseOriginTimestamp = LoadTimestamp(payload + kBodyOffset);
            break;

        case kPtpMsgtypePdelayResp:
            if (payload_len >= kBodyOffset + sizeof(Timestamp))
            {
                msg.pdelay_resp.requestReceiptTimestamp = LoadTimestamp(payload + kBodyOffset);
                // Also parse requestingPortIdentity (8-byte ClockIdentity + 2-byte portNumber)
                // so that ComputeAndStoreUnlocked() can verify the response is addressed to us.
                constexpr std::size_t kPidOffset = kBodyOffset + sizeof(Timestamp);  // = 44
                if (payload_len >= kPidOffset + 10U)
                {
                    std::memcpy(msg.pdelay_resp.requestingPortIdentity.clockIdentity.id,
                                payload + kPidOffset, 8);
                    msg.pdelay_resp.requestingPortIdentity.portNumber = LoadU16(payload + kPidOffset + 8U);
                }
            }
            break;

        case kPtpMsgtypePdelayRespFollowUp:
            if (payload_len >= kBodyOffset + sizeof(Timestamp))
            {
                msg.pdelay_resp_fup.responseOriginReceiptTimestamp = LoadTimestamp(payload + kBodyOffset);
                constexpr std::size_t kPidOffset = kBodyOffset + sizeof(Timestamp);  // = 44
                if (payload_len >= kPidOffset + 10U)
                {
                    std::memcpy(msg.pdelay_resp_fup.requestingPortIdentity.clockIdentity.id,
                                payload + kPidOffset, 8);
                    msg.pdelay_resp_fup.requestingPortIdentity.portNumber =
                        LoadU16(payload + kPidOffset + 8U);
                }
            }
            break;

        default:
            break;
    }

    return true;
}

}  // namespace details
}  // namespace ts
}  // namespace score
