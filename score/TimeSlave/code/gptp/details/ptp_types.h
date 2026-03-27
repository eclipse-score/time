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
#ifndef SCORE_TIMESLAVE_CODE_GPTP_DETAILS_PTP_TYPES_H
#define SCORE_TIMESLAVE_CODE_GPTP_DETAILS_PTP_TYPES_H

#include <netinet/in.h>
#include <cstdint>
#include <cstring>

#ifndef _QNX_PLAT
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

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

namespace score
{
namespace ts
{
namespace details
{

// ─── EtherType constants ────────────────────────────────────────────────────
constexpr int kEthP1588 = 0x88F7;
constexpr int kEthP8021Q = 0x8100;

// ─── MAC / buffer sizes ─────────────────────────────────────────────────────
constexpr int kMacAddrLen = 6;
constexpr int kVlanTagLen = 4;

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

// ─── MAC addresses ───────────────────────────────────────────────────────────
constexpr const char* kPtpSrcMac = "02:00:00:FF:00:11";
constexpr const char* kPtpDstMac = "01:80:C2:00:00:0E";

// ─── Control field ───────────────────────────────────────────────────────────
enum ControlField : std::uint8_t
{
    kCtlSync = 0,
    kCtlDelayReq = 1,
    kCtlFollowUp = 2,
    kCtlDelayResp = 3,
    kCtlManagement = 4,
    kCtlOther = 5
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

// ─── PTP wire structures (all PACKED) ────────────────────────────────────────
struct PACKED ClockIdentity
{
    std::uint8_t id[8]{};
};

struct PACKED PortIdentity
{
    ClockIdentity clockIdentity;
    std::uint16_t portNumber{0};
};

struct PACKED Timestamp
{
    std::uint16_t seconds_msb{0};
    std::uint32_t seconds_lsb{0};
    std::uint32_t nanoseconds{0};
};

struct PACKED PTPHeader
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

struct PACKED SyncBody
{
    PTPHeader ptpHdr{};
    Timestamp originTimestamp{};
};

struct PACKED FollowUpBody
{
    PTPHeader ptpHdr{};
    Timestamp preciseOriginTimestamp{};
};

struct PACKED PdelayReqBody
{
    PTPHeader ptpHdr{};
    Timestamp requestReceiptTimestamp{};
    PortIdentity reserved{};
};

struct PACKED PdelayRespBody
{
    PTPHeader ptpHdr{};
    Timestamp responseOriginTimestamp{};
    PortIdentity requestingPortIdentity{};
};

struct PACKED PdelayRespFollowUpBody
{
    PTPHeader ptpHdr{};
    Timestamp responseOriginReceiptTimestamp{};
    PortIdentity requestingPortIdentity{};
};

struct PACKED RawMessageData
{
    std::uint8_t buffer[1500]{};
};

struct PTPMessage
{
    union PACKED
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
};

static_assert(sizeof(PTPMessage) <= 1600, "PTPMessage too large");

// ─── Timestamp conversion helpers ────────────────────────────────────────────
inline TmvT TimestampToTmv(const Timestamp& ts) noexcept
{
    const std::uint64_t sec =
        (static_cast<std::uint64_t>(ts.seconds_msb) << 32U) | static_cast<std::uint64_t>(ts.seconds_lsb);
    return TmvT{static_cast<std::int64_t>(sec * static_cast<std::uint64_t>(kNsPerSec) + ts.nanoseconds)};
}

inline Timestamp TmvToTimestamp(const TmvT& x) noexcept
{
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
    return TmvT{corr >> 16};
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

#endif  // SCORE_TIMESLAVE_CODE_GPTP_DETAILS_PTP_TYPES_H
