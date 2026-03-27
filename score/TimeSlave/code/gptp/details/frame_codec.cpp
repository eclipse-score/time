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
#include "score/TimeSlave/code/gptp/details/frame_codec.h"

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

int Str2Mac(const char* s, unsigned char mac[kMacAddrLen]) noexcept
{
    unsigned int b[kMacAddrLen]{};
    if (std::sscanf(s, "%x:%x:%x:%x:%x:%x",
                    &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != kMacAddrLen)
    {
        return -1;
    }
    for (int i = 0; i < kMacAddrLen; ++i)
        mac[i] = static_cast<unsigned char>(b[i]);
    return 0;
}

}  // namespace

bool FrameCodec::ParseEthernetHeader(const std::uint8_t* frame,
                                            int                 frame_len,
                                            int&                ptp_offset) const
{
    const int kEthHdrLen = static_cast<int>(sizeof(ethhdr));
    if (frame_len < kEthHdrLen)
        return false;

    ethhdr hdr{};
    std::memcpy(&hdr, frame, sizeof(hdr));

    const auto etype = static_cast<unsigned short>(ntohs(hdr.h_proto));

    if (etype == static_cast<unsigned short>(kEthP8021Q))
    {
        // Skip 4-byte VLAN tag; re-read EtherType
        if (frame_len < kEthHdrLen + kVlanTagLen + 2)
            return false;
        const uint16_t inner_etype_be =
            *reinterpret_cast<const uint16_t*>(frame + kEthHdrLen + kVlanTagLen);
        if (ntohs(inner_etype_be) != static_cast<uint16_t>(kEthP1588))
            return false;
        ptp_offset = kEthHdrLen + kVlanTagLen;
        return true;
    }

    if (etype != static_cast<unsigned short>(kEthP1588))
        return false;

    ptp_offset = kEthHdrLen;
    return true;
}

bool FrameCodec::AddEthernetHeader(std::uint8_t* buf,
                                          unsigned int& buf_len) const
{
    constexpr unsigned int kMaxFrameSize = 2048U;
    const unsigned int kHdrLen = static_cast<unsigned int>(sizeof(ethhdr));

    if (buf_len + kHdrLen > kMaxFrameSize)
        return false;

    std::memmove(buf + kHdrLen, buf, buf_len);

    auto* hdr = reinterpret_cast<ethhdr*>(buf);
    if (Str2Mac(kPtpSrcMac, hdr->h_source) != 0 ||
        Str2Mac(kPtpDstMac, hdr->h_dest) != 0)
    {
        return false;
    }
    hdr->h_proto = htons(static_cast<uint16_t>(kEthP1588));

    buf_len += kHdrLen;
    return true;
}

}  // namespace details
}  // namespace ts
}  // namespace score
