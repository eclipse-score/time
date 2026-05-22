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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_MESSAGE_PARSER_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_MESSAGE_PARSER_H

#include "score/time_slave/src/gptp/details/ptp_types.h"

#include <cstddef>
#include <cstdint>

namespace score
{
namespace ts
{
namespace details
{

/**
 * @brief IEEE 802.1AS / 1588-v2 message parser.
 *
 * Decoupled from the socket layer: callers feed the PTP payload (post
 * Ethernet-header stripping) as a byte buffer and receive a fully populated
 * PTPMessage.
 */
class GptpMessageParser final
{
  public:
    /**
     * @brief Parse @p payload_len bytes at @p payload into @p msg.
     *
     * Populates the PTPHeader union fields and the message-type-specific body
     * fields (Timestamps, PortIdentity, correctionField).  Does NOT touch the
     * hardware-timestamp fields (recvHardwareTS, sendHardwareTS) — those are
     * filled by the caller after the socket recv.
     *
     * @return true if the payload contains a valid IEEE 1588 / 802.1AS header.
     */
    bool Parse(const std::uint8_t* payload, std::size_t payload_len, PTPMessage& msg) const;
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_MESSAGE_PARSER_H
