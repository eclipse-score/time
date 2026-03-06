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
#pragma once
#include <cstdint>
#include <netinet/in.h>
#include "tsync_types.hpp"

#ifdef _QNX_PLAT
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
