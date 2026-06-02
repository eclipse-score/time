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
#include "score/time_slave/src/gptp/details/frame_codec.h"

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

constexpr std::array<std::uint8_t, kMacAddrLen> kPtpDstMacBytes = {
    0x01U, 0x80U, 0xC2U, 0x00U, 0x00U, 0x0EU};

constexpr std::size_t kVlanTciLen = 2U;

}  // namespace

bool FrameCodec::ParseEthernetHeader(const std::uint8_t* frame, int frame_len, int& ptp_offset) const
{
    // Convert to size_t once (after the negative guard) to avoid signed/unsigned
    // comparisons when mixing frame_len (int) with size constants (std::size_t).
    if (frame_len <= 0)
        return false;
    const std::size_t len = static_cast<std::size_t>(frame_len);

    constexpr std::size_t kEthHdrLen = sizeof(ethhdr);
    if (len < kEthHdrLen)
        return false;

    ethhdr hdr{};
    std::memcpy(&hdr, frame, sizeof(hdr));

    const auto etype = static_cast<std::uint16_t>(ntohs(hdr.h_proto));

    if (etype == kEthP8021Q)
    {
        // After the 14-byte ethhdr, the 802.1Q VLAN overhead is:
        //   offset 14–15: TCI (2 bytes)
        //   offset 16–17: inner EtherType (2 bytes)   ← read from here
        //   offset 18+  : PTP payload                 ← ptp_offset
        if (len < kEthHdrLen + kVlanTagLen + 2U)
            return false;
        std::uint16_t inner_etype_be{};
        std::memcpy(&inner_etype_be, frame + kEthHdrLen + kVlanTciLen, sizeof(inner_etype_be));
        if (static_cast<std::uint16_t>(ntohs(inner_etype_be)) != kEthP1588)
            return false;
        ptp_offset = static_cast<int>(kEthHdrLen + kVlanTagLen);
        return true;
    }

    if (etype != kEthP1588)
        return false;

    ptp_offset = static_cast<int>(kEthHdrLen);
    return true;
}

bool FrameCodec::AddEthernetHeader(std::uint8_t* buf,
                                   unsigned int& buf_len,
                                   const std::array<std::uint8_t, kMacAddrLen>& src_mac,
                                   std::size_t buf_capacity) const
{
    const unsigned int kHdrLen = static_cast<unsigned int>(sizeof(ethhdr));

    if (buf_capacity < kHdrLen || buf_len > static_cast<unsigned int>(buf_capacity) - kHdrLen)
        return false;

    std::memmove(buf + kHdrLen, buf, buf_len);

    auto* hdr = reinterpret_cast<ethhdr*>(buf);
    std::memcpy(hdr->h_dest, kPtpDstMacBytes.data(), kMacAddrLen);
    std::memcpy(hdr->h_source, src_mac.data(), kMacAddrLen);
    hdr->h_proto = htons(static_cast<uint16_t>(kEthP1588));

    buf_len += kHdrLen;
    return true;
}

}  // namespace details
}  // namespace ts
}  // namespace score
