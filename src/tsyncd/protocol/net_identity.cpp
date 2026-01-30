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
#include "net_identity.hpp"

#include <cstdio>
#include <cstring>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef _QNX710_
#include <net/if.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#else
#include <linux/if.h>
#include <linux/if_arp.h>
#endif

namespace tsyncd
{
    namespace
    {
    #ifdef _QNX710_
        int iface_mac(const char *name, unsigned char *out_mac, int &out_len)
        {
            if (!name || !out_mac)
                return -1;

            ::ifaddrs *ifaddr = nullptr;
            if (::getifaddrs(&ifaddr) != 0 || !ifaddr)
                return -1;

            int result = -1;
            for (::ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next)
            {
                if (!ifa->ifa_name || !ifa->ifa_addr)
                    continue;
                if (std::strcmp(ifa->ifa_name, name) != 0)
                    continue;
                if (ifa->ifa_addr->sa_family != AF_LINK)
                    continue;

                auto *sdl = reinterpret_cast<sockaddr_dl *>(ifa->ifa_addr);
                const unsigned char *mac = reinterpret_cast<const unsigned char *>(LLADDR(sdl));
                const int len = static_cast<int>(sdl->sdl_alen);
                if (len == 6 || len == 8)
                {
                    std::memcpy(out_mac, mac, static_cast<std::size_t>(len));
                    out_len = len;
                    result = 0;
                    break;
                }
            }

            ::freeifaddrs(ifaddr);
            return result;
        }
    #else
        int iface_mac(const char *name, unsigned char *out_mac, int &out_len)
        {
            if (!name || !out_mac)
                return -1;

            ::ifreq ifr{};
            std::snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);

            const int fd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (fd < 0)
                return -1;

            const int rc = ::ioctl(fd, SIOCGIFHWADDR, &ifr);
            ::close(fd);
            if (rc < 0)
                return -1;

            std::memcpy(out_mac, ifr.ifr_hwaddr.sa_data, 6);
            out_len = 6;
            return 0;
        }
    #endif
    }

    int generate_clock_identity(ClockIdentity &ci, const char *iface_name)
    {
        unsigned char mac[8]{};
        int len = 0;
        if (iface_mac(iface_name, mac, len) != 0)
            return -1;

        if (len == 6)
        {
            ci.id[0] = mac[0];
            ci.id[1] = mac[1];
            ci.id[2] = mac[2];
            ci.id[3] = 0xFF;
            ci.id[4] = 0xFE;
            ci.id[5] = mac[3];
            ci.id[6] = mac[4];
            ci.id[7] = mac[5];
            return 0;
        }
        if (len == 8)
        {
            std::memcpy(ci.id, mac, 8);
            return 0;
        }
        return -1;
    }
}
