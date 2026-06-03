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
#include "score/time_slave/src/gptp/details/network_identity_impl.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/types.h>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

/// Read the MAC address of @p iface_name into @p out_mac (6 or 8 bytes).
/// @return Number of MAC bytes written, or -1 on failure.
int ReadMac(const char* iface_name, unsigned char out_mac[8]) noexcept
{
    if (!iface_name || !out_mac)
        return -1;

    ::ifaddrs* ifaddr = nullptr;
    if (::getifaddrs(&ifaddr) != 0 || ifaddr == nullptr)
        return -1;

    int result = -1;
    for (::ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_name || !ifa->ifa_addr)
            continue;
        if (std::strcmp(ifa->ifa_name, iface_name) != 0)
            continue;
        if (ifa->ifa_addr->sa_family != AF_LINK)
            continue;

        const auto* sdl = reinterpret_cast<const ::sockaddr_dl*>(ifa->ifa_addr);
        const auto* mac = reinterpret_cast<const unsigned char*>(LLADDR(sdl));
        const int len = static_cast<int>(sdl->sdl_alen);
        if (len == 6 || len == 8)
        {
            std::memcpy(out_mac, mac, static_cast<std::size_t>(len));
            result = len;
            break;
        }
    }

    ::freeifaddrs(ifaddr);
    return result;
}

}  // namespace

bool NetworkIdentityImpl::Resolve(const std::string& iface_name)
{
    unsigned char mac[8]{};
    const int len = ReadMac(iface_name.c_str(), mac);

    if (len == 6)
    {
        // EUI-48 → EUI-64: insert 0xFF 0xFE after the OUI (octets 0-2)
        identity_.id[0] = mac[0];
        identity_.id[1] = mac[1];
        identity_.id[2] = mac[2];
        identity_.id[3] = 0xFFU;
        identity_.id[4] = 0xFEU;
        identity_.id[5] = mac[3];
        identity_.id[6] = mac[4];
        identity_.id[7] = mac[5];
        return true;
    }
    if (len == 8)
    {
        std::memcpy(identity_.id, mac, 8);
        return true;
    }
    return false;
}

}  // namespace details
}  // namespace ts
}  // namespace score
