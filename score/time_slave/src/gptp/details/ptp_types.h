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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_PTP_TYPES_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_PTP_TYPES_H

#include <netinet/in.h>
#include <cstdint>
#include <cstring>
#include <limits>

#ifndef __QNXNTO__
#include <linux/if_ether.h>
#else
// Minimal ethhdr definition for QNX
struct ethhdr
{
    unsigned char h_dest[6];
    unsigned char h_source[6];
    uint16_t h_proto;
};
#endif

#define SCORE_TS_PACKED __attribute__((packed))

namespace score
{
namespace ts
{
namespace details
{

// ─── EtherType constants ────────────────────────────────────────────────────
constexpr std::uint16_t kEthP1588  = 0x88F7U;
constexpr std::uint16_t kEthP8021Q = 0x8100U;

// ─── MAC / buffer sizes ─────────────────────────────────────────────────────
constexpr std::size_t kMacAddrLen = 6U;
constexpr std::size_t kVlanTagLen = 4U;

// ─── PTP message-type codes ─────────────────────────────────────────────────
constexpr std::uint8_t kPtpMsgtypeSync = 0x0;
constexpr std::uint8_t kPtpMsgtypePdelayReq = 0x2;
constexpr std::uint8_t kPtpMsgtypePdelayResp = 0x3;
constexpr std::uint8_t kPtpMsgtypeFollowUp = 0x8;
constexpr std::uint8_t kPtpMsgtypePdelayRespFollowUp = 0xA;

// ─── PTP header constants ────────────────────────────────────────────────────
constexpr std::uint8_t kPtpTransportSpecific = (1U << 4U);
constexpr std::uint8_t kPtpVersion = 2U;

constexpr std::int64_t kNsPerSec = 1'000'000'000LL;

// ─── Control field ───────────────────────────────────────────────────────────
enum class ControlField : std::uint8_t
{
    kSync = 0,
    kDelayReq = 1,
    kFollowUp = 2,
    kDelayResp = 3,
    kManagement = 4,
    kOther = 5
};

// ─── State machine states ────────────────────────────────────────────────────
enum class SyncState : std::uint8_t
{
    kEmpty,
    kHaveSync,
    kHaveFup
};

// ─── Time value type ─────────────────────────────────────────────────────────
struct TmvT
{
    std::int64_t ns{0};
};

// ─── PTP wire structures (all SCORE_TS_PACKED) ───────────────────────────────
struct SCORE_TS_PACKED ClockIdentity
{
    std::uint8_t id[8]{};
};

struct SCORE_TS_PACKED PortIdentity
{
    ClockIdentity clockIdentity;
    std::uint16_t portNumber{0};
};

struct SCORE_TS_PACKED Timestamp
{
    std::uint16_t seconds_msb{0};
    std::uint32_t seconds_lsb{0};
    std::uint32_t nanoseconds{0};
};

struct SCORE_TS_PACKED PTPHeader
{
    std::uint8_t tsmt{0};
    std::uint8_t version{0};
    std::uint16_t messageLength{0};
    std::uint8_t domainNumber{0};
    std::uint8_t reserved1{0};
    std::uint8_t flagField[2]{};
    std::int64_t correctionField{0};
    std::uint32_t reserved2{0};
    PortIdentity sourcePortIdentity{};
    std::uint16_t sequenceId{0};
    std::uint8_t controlField{0};
    std::int8_t logMessageInterval{0};
};

struct SCORE_TS_PACKED SyncBody
{
    PTPHeader ptpHdr{};
    Timestamp originTimestamp{};
};

struct SCORE_TS_PACKED FollowUpBody
{
    PTPHeader ptpHdr{};
    Timestamp preciseOriginTimestamp{};
};

struct SCORE_TS_PACKED PdelayReqBody
{
    PTPHeader ptpHdr{};
    Timestamp requestReceiptTimestamp{};
    PortIdentity reserved{};
};

struct SCORE_TS_PACKED PdelayRespBody
{
    PTPHeader ptpHdr{};
    Timestamp requestReceiptTimestamp{};  ///< IEEE 802.1AS: t₂ — time the remote peer received our PdelayReq
    PortIdentity requestingPortIdentity{};
};

struct SCORE_TS_PACKED PdelayRespFollowUpBody
{
    PTPHeader ptpHdr{};
    Timestamp responseOriginReceiptTimestamp{};
    PortIdentity requestingPortIdentity{};
};

struct SCORE_TS_PACKED RawMessageData
{
    std::uint8_t buffer[1500]{};
};

struct PTPMessage
{
    union SCORE_TS_PACKED
    {
        PTPHeader ptpHdr;
        SyncBody sync;
        FollowUpBody follow_up;
        PdelayReqBody pdelay_req;
        PdelayRespBody pdelay_resp;
        PdelayRespFollowUpBody pdelay_resp_fup;
        RawMessageData data;
    };

    std::uint8_t msgtype{0};
    TmvT sendHardwareTS{};
    TmvT parseMessageTs{};
    TmvT recvHardwareTS{};
    std::int64_t recvMonoNs{0};  // CLOCK_MONOTONIC at packet reception; set for Sync only
};

static_assert(sizeof(PTPMessage) <= 1600, "PTPMessage too large");

// ─── Timestamp conversion helpers ────────────────────────────────────────────
inline TmvT TimestampToTmv(const Timestamp& ts) noexcept
{
    const std::uint64_t sec =
        (static_cast<std::uint64_t>(ts.seconds_msb) << 32U) | static_cast<std::uint64_t>(ts.seconds_lsb);
    constexpr std::uint64_t kMaxNs =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    constexpr std::uint64_t kMaxSec = kMaxNs / static_cast<std::uint64_t>(kNsPerSec);
    if (sec > kMaxSec)
        return TmvT{};
    const std::uint64_t total_ns = sec * static_cast<std::uint64_t>(kNsPerSec) + ts.nanoseconds;
    if (total_ns > kMaxNs)
        return TmvT{};
    return TmvT{static_cast<std::int64_t>(total_ns)};
}

inline Timestamp TmvToTimestamp(const TmvT& x) noexcept
{
    if (x.ns < 0)
        return Timestamp{};  // negative timestamps are invalid on the wire
    Timestamp t{};
    const std::uint64_t sec = static_cast<std::uint64_t>(x.ns) / 1'000'000'000ULL;
    const std::uint64_t nsec = static_cast<std::uint64_t>(x.ns) % 1'000'000'000ULL;
    t.seconds_lsb = static_cast<std::uint32_t>(sec & 0xFFFFFFFFULL);
    t.seconds_msb = static_cast<std::uint16_t>((sec >> 32U) & 0xFFFFULL);
    t.nanoseconds = static_cast<std::uint32_t>(nsec);
    return t;
}

inline TmvT CorrectionToTmv(std::int64_t corr) noexcept
{
    return TmvT{corr / 65536LL};
}

inline std::uint64_t ClockIdentityToU64(const ClockIdentity& ci) noexcept
{
    std::uint64_t v{0};
    std::memcpy(&v, ci.id, sizeof(v));
    return v;
}

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_PTP_TYPES_H
