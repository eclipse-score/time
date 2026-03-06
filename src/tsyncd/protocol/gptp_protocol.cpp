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
#include "gptp_protocol.hpp"
#include <arpa/inet.h>
#include <cstring>

#ifndef _QNX_PLAT
#include <byteswap.h>
#endif

namespace tsyncd
{
    namespace
    {
        static inline std::uint16_t load_u16(const unsigned char *p)
        {
            std::uint16_t v;
            std::memcpy(&v, p, sizeof(v));
            return ntohs(v);
        }
        static inline std::uint32_t load_u32(const unsigned char *p)
        {
            std::uint32_t v;
            std::memcpy(&v, p, sizeof(v));
            return ntohl(v);
        }
        static inline std::uint64_t load_be64(const unsigned char *p)
        {
            std::uint64_t v;
            std::memcpy(&v, p, sizeof(v));
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            v = __builtin_bswap64(v);
    #endif
            return v;
        }
    }

    bool parse_gptp_message(const unsigned char *buffer, std::size_t buffer_len, PTPMessage &ptp_message)
    {
        if (buffer_len < sizeof(PTPHeader))
            return false;

        ptp_message.ptpHdr.tsmt = buffer[0];
        ptp_message.ptpHdr.version = buffer[1];
        ptp_message.ptpHdr.messageLength = load_u16(buffer + 2);
        ptp_message.ptpHdr.domainNumber = buffer[4];
        ptp_message.ptpHdr.reserved1 = buffer[5];
        std::memcpy(ptp_message.ptpHdr.flagField, buffer + 6, 2);
        ptp_message.ptpHdr.correctionField = static_cast<std::int64_t>(load_be64(buffer + 8));
        ptp_message.ptpHdr.reserved2 = load_u32(buffer + 16);

        std::memcpy(ptp_message.ptpHdr.sourcePortIdentity.clockIdentity.id, buffer + 20, 8);
        ptp_message.ptpHdr.sourcePortIdentity.portNumber = load_u16(buffer + 28);

        ptp_message.ptpHdr.sequenceId = load_u16(buffer + 30);
        ptp_message.ptpHdr.controlField = buffer[32];
        ptp_message.ptpHdr.logMessageInterval = static_cast<std::int8_t>(buffer[33]);

        ptp_message.msgtype = ptp_message.ptpHdr.tsmt & 0x0F;

        if (ptp_message.msgtype == PTP_MSGTYPE_FOLLOW_UP)
        {
            ptp_message.follow_up.preciseOriginTimestamp.seconds_msb = load_u16(buffer + 34);
            ptp_message.follow_up.preciseOriginTimestamp.seconds_lsb = load_u32(buffer + 36);
            ptp_message.follow_up.preciseOriginTimestamp.nanoseconds = load_u32(buffer + 40);
        }
        else if (ptp_message.msgtype == PTP_MSGTYPE_PDELAY_RESP)
        {
            ptp_message.pdelay_resp.responseOriginTimestamp.seconds_msb = load_u16(buffer + 34);
            ptp_message.pdelay_resp.responseOriginTimestamp.seconds_lsb = load_u32(buffer + 36);
            ptp_message.pdelay_resp.responseOriginTimestamp.nanoseconds = load_u32(buffer + 40);
        }
        else if (ptp_message.msgtype == PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP)
        {
            ptp_message.pdelay_resp_fup.responseOriginReceiptTimestamp.seconds_msb = load_u16(buffer + 34);
            ptp_message.pdelay_resp_fup.responseOriginReceiptTimestamp.seconds_lsb = load_u32(buffer + 36);
            ptp_message.pdelay_resp_fup.responseOriginReceiptTimestamp.nanoseconds = load_u32(buffer + 40);
        }

        return true;
    }

}
