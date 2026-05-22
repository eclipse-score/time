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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_FRAME_CODEC_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_FRAME_CODEC_H

#include "score/time_slave/src/gptp/details/ptp_types.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace score
{
namespace ts
{
namespace details
{

/**
 * @brief Ethernet frame encode/decode for PTP-over-L2.
 *
 * Uses the standard PTP multicast destination MAC 01:80:C2:00:00:0E and
 * EtherType 0x88F7.  VLAN-tagged frames are accepted on receive.
 */
class FrameCodec final
{
  public:
    /**
     * @brief Locate the PTP payload inside a raw Ethernet frame.
     *
     * Handles 802.1Q VLAN-tagged frames transparently.
     *
     * @param frame      Raw frame bytes as received from the socket.
     * @param frame_len  Total length of @p frame in bytes.
     * @param ptp_offset Output: byte offset where the PTP message starts.
     * @return true if @p frame contains a PTP/1588 Ethertype, false otherwise.
     */
    bool ParseEthernetHeader(const std::uint8_t* frame, int frame_len, int& ptp_offset) const;

    /**
     * @brief Prepend an Ethernet header for PTP multicast transmission.
     *
     * Modifies @p buf in-place (shifts payload to make room) and increments
     * @p buf_len by the size of the added header.
     *
     * @param buf           Buffer large enough to hold existing payload plus header.
     * @param buf_len       In/out: payload length → frame length after prepend.
     * @param src_mac       Source MAC address (should be the port's own MAC).
     * @param buf_capacity  Total allocated size of @p buf in bytes; used to detect overflow.
     * @return true on success, false if the buffer would overflow.
     */
    bool AddEthernetHeader(std::uint8_t* buf,
                           unsigned int& buf_len,
                           const std::array<std::uint8_t, kMacAddrLen>& src_mac,
                           std::size_t buf_capacity) const;
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_FRAME_CODEC_H
