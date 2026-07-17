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
#ifndef SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_FRAME_BUILDERS_HPP
#define SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_FRAME_BUILDERS_HPP

#include "score/time_slave/src/gptp/details/ptp_types.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace score
{
namespace ts
{
namespace cit
{

// Append a 14-byte Ethernet II header with EtherType 0x88F7 (IEEE 1588).
inline void AppendEthHeader(std::vector<std::uint8_t>& buf)
{
    const std::uint8_t dst[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};
    const std::uint8_t src[6] = {0x02, 0x00, 0x00, 0xFF, 0x00, 0x11};
    buf.insert(buf.end(), dst, dst + 6);
    buf.insert(buf.end(), src, src + 6);
    buf.push_back(0x88);
    buf.push_back(0xF7);
}

// Append a 34-byte PTP common header (transport-specific | msgtype, version,
// total length, reserved fields, sequence ID, control field).
inline void AppendPtpHeader(std::vector<std::uint8_t>& buf,
                            std::uint8_t msgtype,
                            std::uint16_t seqId,
                            std::uint8_t ctlField = 0)
{
    const std::size_t start = buf.size();
    buf.resize(start + 34, 0);
    std::uint8_t* p = buf.data() + start;
    p[0] = static_cast<std::uint8_t>(0x10U | (msgtype & 0x0FU));
    p[1] = 0x02;
    const std::uint16_t len = htons(static_cast<std::uint16_t>(buf.size() - 14));
    std::memcpy(p + 2, &len, 2);
    const std::uint16_t seq = htons(seqId);
    std::memcpy(p + 30, &seq, 2);
    p[32] = ctlField;
}

// Append a 10-byte PTP Timestamp body (sec_msb=0, sec_lsb, nanoseconds).
inline void AppendTimestamp(std::vector<std::uint8_t>& buf, std::uint32_t sec_lsb, std::uint32_t ns)
{
    const std::uint16_t msb = htons(0U);
    const std::uint32_t sl = htonl(sec_lsb);
    const std::uint32_t n = htonl(ns);
    const std::uint8_t* p;
    p = reinterpret_cast<const std::uint8_t*>(&msb);
    buf.insert(buf.end(), p, p + 2);
    p = reinterpret_cast<const std::uint8_t*>(&sl);
    buf.insert(buf.end(), p, p + 4);
    p = reinterpret_cast<const std::uint8_t*>(&n);
    buf.insert(buf.end(), p, p + 4);
}

// Build a minimal gPTP Sync frame (msgtype=0x0, origin timestamp all-zero).
inline std::vector<std::uint8_t> MakeSyncFrame(std::uint16_t seqId)
{
    std::vector<std::uint8_t> f;
    AppendEthHeader(f);
    AppendPtpHeader(f, score::ts::details::kPtpMsgtypeSync, seqId, 0);
    AppendTimestamp(f, 0, 0);
    return f;
}

// Build a gPTP FollowUp frame carrying the given precise origin timestamp.
inline std::vector<std::uint8_t> MakeFollowUpFrame(std::uint16_t seqId, std::uint32_t sec_lsb, std::uint32_t ns)
{
    std::vector<std::uint8_t> f;
    AppendEthHeader(f);
    AppendPtpHeader(f, score::ts::details::kPtpMsgtypeFollowUp, seqId, 2);
    AppendTimestamp(f, sec_lsb, ns);
    return f;
}

}  // namespace cit
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_FRAME_BUILDERS_HPP
