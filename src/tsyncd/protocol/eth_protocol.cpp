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
#include "eth_protocol.hpp"
#include "tsync_types.hpp"

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>

namespace tsyncd
{
    namespace
    {
        static int str2mac(const char *s, unsigned char mac[MAC_ADDR_LEN])
        {
            unsigned int b[MAC_ADDR_LEN]{};
            const int c = std::sscanf(s, "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
            if (c != MAC_ADDR_LEN)
                return -1;
            for (int i = 0; i < MAC_ADDR_LEN; i++)
                mac[i] = static_cast<unsigned char>(b[i]);
            return 0;
        }
    }

    int add_ethernet_header(unsigned char *buffer, unsigned int &buffer_len)
    {
        // buffer is assumed large enough (caller uses PTPMessage buffer size). Keep simple.
        unsigned char tmp[2048];
        if (buffer_len + sizeof(struct ethhdr) > sizeof(tmp))
            return -1;

        auto *hdr = reinterpret_cast<struct ethhdr *>(tmp);
        if (str2mac(PTP_SRC_MAC, hdr->h_source) != 0 || str2mac(PTP_DST_MAC, hdr->h_dest) != 0)
        {
            return -1;
        }
        hdr->h_proto = htons(ETH_P_1588);

        std::memcpy(tmp + sizeof(struct ethhdr), buffer, buffer_len);
        buffer_len += sizeof(struct ethhdr);
        std::memcpy(buffer, tmp, buffer_len);
        return 0;
    }

    void parse_ethernet_header(const unsigned char *buffer, ethhdr &eth_header, int &offset)
    {
        std::memcpy(&eth_header, buffer, sizeof(ethhdr));
        const unsigned short ether_type = ntohs(eth_header.h_proto);
        if (ether_type == ETH_P_8021Q)
            offset = static_cast<int>(sizeof(struct ethhdr)) + VLAN_TAG_LEN;
        else
            offset = static_cast<int>(sizeof(struct ethhdr));
    }
}
