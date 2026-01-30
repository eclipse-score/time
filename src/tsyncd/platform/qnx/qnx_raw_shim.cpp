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
#include "qnx_raw_shim.hpp"
#include "tsync_types.hpp"
#include "eth_protocol.hpp"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <devctl.h>

#include <net/if.h>
#include <net/bpf.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>

#include <netdrvr/ptp.h>

static int g_bpf_fd = -1;
static u_int g_bpf_buflen = 0;
static char g_iface_name[IFNAMSIZ] = "";

static int get_hwts_tx_rx(const char *ifname, int dir, const PTPHeader *ptp_hdr, timespec *ts)
{
    if (!ifname || !ptp_hdr || !ts)
    {
        errno = EINVAL;
        return -1;
    }

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        return -1;
    }

    struct
    {
        struct ifdrv ifd;
        ptp_extts_t extts;
    } cmd_time_stamp;

    std::memset(&cmd_time_stamp, 0, sizeof(cmd_time_stamp));

    std::strncpy(cmd_time_stamp.ifd.ifd_name, ifname, sizeof(cmd_time_stamp.ifd.ifd_name) - 1);
    cmd_time_stamp.ifd.ifd_name[sizeof(cmd_time_stamp.ifd.ifd_name) - 1] = '\0';

    cmd_time_stamp.ifd.ifd_cmd = (dir ? PTP_GET_RX_TIMESTAMP : PTP_GET_TX_TIMESTAMP);
    cmd_time_stamp.ifd.ifd_len = sizeof(ptp_extts_t);
    cmd_time_stamp.ifd.ifd_data = &cmd_time_stamp.extts;

    cmd_time_stamp.extts.msg_type = ptp_hdr->tsmt & 0x0f;
    cmd_time_stamp.extts.sport_id = ntohs(ptp_hdr->sourcePortIdentity.portNumber);
    cmd_time_stamp.extts.sequence_id = ntohs(ptp_hdr->sequenceId);

    std::memcpy(cmd_time_stamp.extts.clock_identity,
                ptp_hdr->sourcePortIdentity.clockIdentity.id,
                sizeof(cmd_time_stamp.extts.clock_identity));

    cmd_time_stamp.extts.ts.sec = 0;
    cmd_time_stamp.extts.ts.nsec = 0;

    if (devctl(s, SIOCGDRVSPEC, &cmd_time_stamp, sizeof(cmd_time_stamp), nullptr) == -1)
    {
        close(s);
        return -1;
    }

    close(s);

    if (cmd_time_stamp.extts.ts.sec == 0 && cmd_time_stamp.extts.ts.nsec == 0)
    {
        errno = EAGAIN;
        return -1;
    }

    ts->tv_sec = static_cast<time_t>(cmd_time_stamp.extts.ts.sec);
    ts->tv_nsec = static_cast<long>(cmd_time_stamp.extts.ts.nsec);
    return 0;
}

extern "C" int qnx_raw_open(const char *ifname)
{
    std::cout << "[DEBUG] qnx_raw_open: ifname=" << (ifname ? ifname : "NULL") << std::endl;

    if (!ifname)
    {
        errno = EINVAL;
        return -1;
    }

    strlcpy(g_iface_name, ifname, sizeof(g_iface_name));

    char devpath[256] = {0};
    const char *sock = std::getenv("SOCK");

    if (sock && sock[0])
    {
        std::snprintf(devpath, sizeof(devpath), "%s/dev/bpf0", sock);
    }
    else
    {
        std::snprintf(devpath, sizeof(devpath), "/dev/bpf");
    }

    int fd = open(devpath, O_RDWR);
    if (fd < 0)
    {
        std::cout << "[DEBUG] qnx_raw_open: open " << devpath << " FAILED: "
                  << std::strerror(errno) << std::endl;
        return -1;
    }
    std::cout << "[DEBUG] qnx_raw_open: open " << devpath << " OK, fd=" << fd << std::endl;

    ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    if (ioctl(fd, BIOCSETIF, &ifr) < 0)
    {
        std::cout << "[DEBUG] qnx_raw_open: BIOCSETIF(" << ifr.ifr_name << ") FAILED: "
                  << std::strerror(errno) << std::endl;
        close(fd);
        return -1;
    }
    std::cout << "[DEBUG] qnx_raw_open: BIOCSETIF(" << ifr.ifr_name << ") OK" << std::endl;

    int zero = 0;
    if (ioctl(fd, BIOCSSEESENT, &zero, sizeof(zero)) < 0)
    {
        std::cout << "[DEBUG] qnx_raw_open: BIOCSSEESENT FAILED (ignored): "
                  << std::strerror(errno) << std::endl;
    }

    u_int yes = 1;
    if (ioctl(fd, BIOCIMMEDIATE, &yes) < 0)
    {
        std::cout << "[DEBUG] qnx_raw_open: BIOCIMMEDIATE FAILED (ignored): "
                  << std::strerror(errno) << std::endl;
    }

    if (ioctl(fd, BIOCPROMISC, &yes) < 0)
    {
        std::cout << "[DEBUG] qnx_raw_open: BIOCPROMISC FAILED (ignored): "
                  << std::strerror(errno) << std::endl;
    }

    if (ioctl(fd, BIOCGBLEN, &g_bpf_buflen) < 0)
    {
        std::cout << "[DEBUG] qnx_raw_open: BIOCGBLEN FAILED: "
                  << std::strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    g_bpf_fd = fd;

    std::cout << "[DEBUG] qnx_raw_open: SUCCESS, fd=" << fd << ", ifname=" << g_iface_name << std::endl;
    return fd;
}

extern "C" int qnx_raw_recv(int fd, void *buf, int buf_len, timespec *hwts, int nonblock)
{
    if (fd < 0 || !buf || buf_len <= 0 || !hwts)
    {
        errno = EINVAL;
        return -1;
    }
    if (g_bpf_buflen == 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (nonblock)
    {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags >= 0)
        {
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }
    }

    static unsigned char *bpf_buf = nullptr;
    static ssize_t bpf_n = 0;
    static ssize_t bpf_off = 0;

    if (!bpf_buf)
    {
        bpf_buf = static_cast<unsigned char *>(std::malloc(g_bpf_buflen));
        if (!bpf_buf)
        {
            errno = ENOMEM;
            return -1;
        }
    }

    for (;;)
    {
        if (bpf_off >= bpf_n)
        {
            ssize_t n = read(fd, bpf_buf, g_bpf_buflen);
            if (n < 0)
            {
                return -1;
            }
            if (n == 0)
            {
                if (nonblock)
                {
                    errno = EAGAIN;
                    return -1;
                }
                continue;
            }
            bpf_n = n;
            bpf_off = 0;
        }

        if (bpf_off + static_cast<ssize_t>(sizeof(bpf_hdr)) > bpf_n)
        {
            bpf_off = bpf_n;
            continue;
        }

        auto *bh = reinterpret_cast<bpf_hdr *>(bpf_buf + bpf_off);
        if (bpf_off + bh->bh_hdrlen + bh->bh_caplen > bpf_n)
        {
            bpf_off = bpf_n;
            continue;
        }

        unsigned char *pkt = reinterpret_cast<unsigned char *>(bh) + bh->bh_hdrlen;
        int caplen = static_cast<int>(bh->bh_caplen);

        ssize_t next_off = bpf_off + BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);

        if (caplen >= static_cast<int>(sizeof(::ethhdr)))
        {

            ::ethhdr eth{};
            int ptp_offset = 0;
            tsyncd::parse_ethernet_header(pkt, eth, ptp_offset);

            uint16_t ethertype = ntohs(eth.h_proto);

            if (ethertype == ETH_P_8021Q)
            {
                if (caplen < ptp_offset)
                {
                    bpf_off = next_off;
                    continue;
                }
                ethertype = ntohs(*reinterpret_cast<const uint16_t *>(pkt + static_cast<int>(sizeof(::ethhdr)) + 2));
            }

            if (ethertype == ETH_P_1588 &&
                caplen >= ptp_offset + static_cast<int>(sizeof(PTPHeader)))
            {

                int frame_len = caplen;
                if (frame_len > buf_len)
                {
                    frame_len = buf_len;
                }
                std::memcpy(buf, pkt, static_cast<size_t>(frame_len));

                const auto *ph = reinterpret_cast<const PTPHeader *>(pkt + ptp_offset);

                timespec ts{};
                if (get_hwts_tx_rx(g_iface_name, 1, ph, &ts) < 0)
                {
                    clock_gettime(CLOCK_REALTIME, &ts);
                }
                *hwts = ts;

                bpf_off = next_off;
                return frame_len;
            }
        }

        bpf_off = next_off;
    }
}

extern "C" int qnx_raw_send(int fd, const void *buf, int len, timespec *hwts)
{
    if (fd < 0 || !buf || len <= 0 || !hwts)
    {
        errno = EINVAL;
        return -1;
    }

    unsigned char frame[ETHER_HDR_LEN + 1500];
    unsigned int frame_len = static_cast<unsigned int>(len);

    if (frame_len > 1500)
    {
        errno = EMSGSIZE;
        return -1;
    }

    std::memcpy(frame, buf, frame_len);

    ssize_t n = write(fd, frame, frame_len);
    if (n < 0)
    {
        return -1;
    }

    const auto *ph = reinterpret_cast<const PTPHeader *>(frame + ETHER_HDR_LEN);
    timespec ts{};
    if (get_hwts_tx_rx(g_iface_name, 0, ph, &ts) < 0)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
    }
    *hwts = ts;

    return len;
}

extern "C" int qnx_phc_open(const char *phc_dev)
{
    if (phc_dev && phc_dev[0] != '\0' && phc_dev[0] != '/')
    {
        strlcpy(g_iface_name, phc_dev, sizeof(g_iface_name));
    }
    return 0;
}

extern "C" int qnx_phc_adjtime_step(int phc_fd, long long offset_ns)
{
    (void)phc_fd;

    if (offset_ns == 0)
    {
        std::cout << "[DEBUG] qnx_phc_adjtime_step: offset_ns == 0, no-op" << std::endl;
        return 0;
    }

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        std::cout << "[DEBUG] qnx_phc_adjtime_step: socket() FAILED: "
                  << std::strerror(errno) << std::endl;
        return -1;
    }

    struct
    {
        ifdrv ifd;
        ptp_time_t tm;
    } cmd;

    std::memset(&cmd, 0, sizeof(cmd));

    std::strncpy(cmd.ifd.ifd_name, g_iface_name, sizeof(cmd.ifd.ifd_name) - 1);
    cmd.ifd.ifd_name[sizeof(cmd.ifd.ifd_name) - 1] = '\0';
    cmd.ifd.ifd_len = sizeof(cmd.tm);
    cmd.ifd.ifd_data = &cmd.tm;

    cmd.ifd.ifd_cmd = PTP_GET_TIME;
    if (devctl(s, SIOCGDRVSPEC, &cmd, sizeof(cmd), nullptr) == -1)
    {
        std::cout << "[DEBUG] qnx_phc_adjtime_step: GET_TIME FAILED on " << cmd.ifd.ifd_name
                  << ": " << std::strerror(errno) << std::endl;
        close(s);
        return -1;
    }

    int64_t cur_ns = static_cast<int64_t>(cmd.tm.sec) * NS_PER_SEC +
                     static_cast<int64_t>(cmd.tm.nsec);

    int64_t new_ns = cur_ns + offset_ns;

    if (new_ns < static_cast<int64_t>(INT32_MIN) * NS_PER_SEC)
    {
        new_ns = static_cast<int64_t>(INT32_MIN) * NS_PER_SEC;
    }
    if (new_ns > static_cast<int64_t>(INT32_MAX) * NS_PER_SEC)
    {
        new_ns = static_cast<int64_t>(INT32_MAX) * NS_PER_SEC;
    }

    int32_t new_sec = static_cast<int32_t>(new_ns / NS_PER_SEC);
    int32_t new_nsec = static_cast<int32_t>(new_ns % NS_PER_SEC);
    if (new_nsec < 0)
    {
        new_nsec += NS_PER_SEC;
        new_sec -= 1;
    }

    cmd.tm.sec = new_sec;
    cmd.tm.nsec = new_nsec;

    cmd.ifd.ifd_cmd = PTP_SET_TIME;
    if (devctl(s, SIOCGDRVSPEC, &cmd, sizeof(cmd), nullptr) == -1)
    {
        std::cout << "[DEBUG] qnx_phc_adjtime_step: SET_TIME FAILED on " << cmd.ifd.ifd_name
                  << ": " << std::strerror(errno) << std::endl;
        close(s);
        return -1;
    }

    std::cout << "[DEBUG] qnx_phc_adjtime_step: SUCCESS, sec=" << new_sec
              << ", nsec=" << new_nsec << std::endl;

    close(s);
    return 0;
}

extern "C" int qnx_phc_adjfreq_ppb(int phc_fd, long long freq_ppb)
{
    (void)phc_fd;
    (void)freq_ppb;
    return 0;
}
