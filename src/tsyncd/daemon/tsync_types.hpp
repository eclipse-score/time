/********************************************************************************************************
# Copyright ©, 2026, ECARX. All rights reserved. This statement is the current and authoritative version.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: chenhao.gao chenhao.gao@ecarxgroup.com
*********************************************************************************************************/
#pragma once
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef _QNX710_
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#else
constexpr int ETH_P_1588 = 0x88F7;
constexpr int ETH_P_8021Q = 0x8100;
#endif

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

constexpr const char *kDefaultPhcDevice = "/dev/ptp0";
constexpr const char *kDefaultIfaceName = "eth0";

constexpr const char *PTP_SRC_MAC = "02:00:00:FF:00:11";
constexpr const char *PTP_DST_MAC = "01:80:C2:00:00:0E";

constexpr int MAC_ADDR_LEN = 6;
constexpr int VLAN_TAG_LEN = 4;
constexpr int GUID_OFFSET = 36;
constexpr int GUID_LEN = 8;
constexpr int MAX_MSG_QUEUE_SIZE = 5;
constexpr std::int64_t NS_PER_SEC = 1'000'000'000LL;

constexpr int NO_ERROR = 0;

constexpr std::uint8_t PTP_TRANSPORT_SPECIFIC = (1 << 4);
constexpr std::uint8_t PTP_VERSION = 2;

constexpr std::uint8_t PTP_MSGTYPE_SYNC = 0x0;
constexpr std::uint8_t PTP_MSGTYPE_DELAY_REQ = 0x1;
constexpr std::uint8_t PTP_MSGTYPE_PDELAY_REQ = 0x2;
constexpr std::uint8_t PTP_MSGTYPE_PDELAY_RESP = 0x3;
constexpr std::uint8_t PTP_MSGTYPE_FOLLOW_UP = 0x8;
constexpr std::uint8_t PTP_MSGTYPE_DELAY_RESP = 0x9;
constexpr std::uint8_t PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP = 0xA;

enum ControlField : std::uint8_t
{
    CTL_SYNC,
    CTL_DELAY_REQ,
    CTL_FOLLOW_UP,
    CTL_DELAY_RESP,
    CTL_MANAGEMENT,
    CTL_OTHER
};

enum class TsyncState : std::uint8_t
{
    kEmpty,
    kHaveSync,
    kHaveFup
};

enum class TsyncEvent : std::uint8_t
{
    kOther,
    kRecvSync,
    kRecvFup
};

struct ClockIdentity
{
    std::uint8_t id[8];
};

struct tmv_t
{
    std::int64_t ns;
};

struct PACKED PortIdentity
{
    ClockIdentity clockIdentity;
    std::uint16_t portNumber;
};

struct PACKED Timestamp
{
    std::uint16_t seconds_msb;
    std::uint32_t seconds_lsb;
    std::uint32_t nanoseconds;
};

struct PACKED PTPHeader
{
    std::uint8_t tsmt;
    std::uint8_t version;
    std::uint16_t messageLength;
    std::uint8_t domainNumber;
    std::uint8_t reserved1;
    std::uint8_t flagField[2];
    std::int64_t correctionField;
    std::uint32_t reserved2;
    PortIdentity sourcePortIdentity;
    std::uint16_t sequenceId;
    std::uint8_t controlField;
    std::int8_t logMessageInterval;
};

struct PACKED SyncBody
{
    PTPHeader ptpHdr;
    Timestamp originTimestamp;
};
struct PACKED FollowUpBody
{
    PTPHeader ptpHdr;
    Timestamp preciseOriginTimestamp;
    std::uint8_t suffix[0];
};
struct PACKED PdelayReqBody
{
    PTPHeader ptpHdr;
    Timestamp requestReceiptTimestamp;
    PortIdentity reserved;
};
struct PACKED PdelayRespBody
{
    PTPHeader ptpHdr;
    Timestamp responseOriginTimestamp;
    PortIdentity requestingPortIdentity;
};
struct PACKED PdelayRespFollowUpBody
{
    PTPHeader ptpHdr;
    Timestamp responseOriginReceiptTimestamp;
    PortIdentity requestingPortIdentity;
};

struct PACKED MessageData
{
    std::uint8_t buffer[1500];
};

struct PTPMessage
{
    union
    {
        PTPHeader ptpHdr;
        SyncBody sync;
        FollowUpBody follow_up;
        PdelayReqBody pdelay_req;
        PdelayRespBody pdelay_resp;
        PdelayRespFollowUpBody pdelay_resp_fup;
        MessageData data;
    } PACKED;

    std::uint8_t msgtype = 0;
    tmv_t sendHardwareTS{0};
    tmv_t parseMessageTs{0};
    tmv_t recvHardwareTS{0};
};

struct PTPMessageQueue
{
    PTPMessage save_message[MAX_MSG_QUEUE_SIZE];
    int save_index = 0;
};

struct Context
{
    int raw_fd = -1;
    int phc_fd = -1;
    clockid_t clk_id{};
    int pdelay_seqnum = 0;
    std::int64_t path_delay = 0;
    std::int64_t last_master_ts = 0;

    ClockIdentity clockIdentity{};

    PTPMessageQueue syncMsgQueue{};
    PTPMessageQueue fupMsgQueue{};

    // Simplified state machine cache
    PTPMessage last_sync{};
    PTPMessage last_fup{};

    PTPMessage peer_delay_req{};
    PTPMessage peer_delay_resp{};
    PTPMessage peer_delay_fup{};

    pthread_mutex_t pdelay_lock{};
    TsyncState state = TsyncState::kEmpty;
};
