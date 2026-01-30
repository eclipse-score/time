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
#include <netinet/in.h>
#include "tsync_types.hpp"

#ifdef _QNX710_
struct ethhdr {
    unsigned char h_dest[MAC_ADDR_LEN];
    unsigned char h_source[MAC_ADDR_LEN];
    uint16_t h_proto;
};
#else
#include <linux/if_ether.h>
#endif

namespace tsyncd
{
    int add_ethernet_header(unsigned char *buffer, unsigned int &buffer_len);
    void parse_ethernet_header(const unsigned char *buffer, ethhdr &eth_header, int &offset);
}
