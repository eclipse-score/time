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

#include <net/if.h>
#include <net/bpf.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>

// QNX SDP 8.0: PTP API moved to io-sock/ptp.h. We cannot include that header
// directly because it redefines struct PortIdentity (also in tsync_types.hpp).
// Only the constants and struct ptp_time are used here; define them inline.
// Source: target/qnx/usr/include/io-sock/ptp.h (QNX SDP 8.0)
#define PTP_GET_TIME  0x102
#define PTP_SET_TIME  0x103
struct ptp_time {
    int64_t sec;
    int32_t nsec;
};

// BPF buffer size: use static buffer to avoid malloc/free in thread_local
// Typical BPF buffer size is 32KB-64KB; we use 64KB as safe maximum
static constexpr std::size_t kMaxBpfBufSize = 65536;

struct QnxRawContext
{
    int bpf_fd = -1;
    u_int bpf_buflen = 0;
    char iface_name[IFNAMSIZ] = "";
    unsigned char bpf_buf[kMaxBpfBufSize];  // Static buffer, no malloc/free needed
    ssize_t bpf_n = 0;
    ssize_t bpf_off = 0;
    bool bpf_buf_initialized = false;
    unsigned char tx_frame[ETHER_HDR_LEN + 1500]{};  // TX frame buffer to avoid stack allocation

    // Destructor ensures proper cleanup of file descriptor when thread exits
    // Critical for thread_local usage - prevents fd leaks when threads terminate
    ~QnxRawContext()
    {
        if (bpf_fd >= 0)
        {
            ::close(bpf_fd);
            bpf_fd = -1;
        }
    }
};

thread_local QnxRawContext g_qnx_ctx;

// Hardware PTP timestamping via netdrvr API is not available in QNX SDP 8.0.
// Always returns -1 so callers fall back to software (CLOCK_REALTIME) timestamps.
static int get_hwts_tx_rx(const char * /*ifname*/, int /*dir*/, const PTPHeader * /*ptp_hdr*/, timespec * /*ts*/)
{
    errno = ENOTSUP;
    return -1;
}

extern "C" int qnx_raw_open(const char *ifname)
{
    std::cout << "[DEBUG] qnx_raw_open: ifname=" << (ifname ? ifname : "NULL") << std::endl;

    if (!ifname)
    {
        errno = EINVAL;
        return -1;
    }

    strlcpy(g_qnx_ctx.iface_name, ifname, sizeof(g_qnx_ctx.iface_name));

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

    if (ioctl(fd, BIOCGBLEN, &g_qnx_ctx.bpf_buflen) < 0)
    {
        std::cout << "[DEBUG] qnx_raw_open: BIOCGBLEN FAILED: "
                  << std::strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Validate buffer size doesn't exceed our static buffer
    if (g_qnx_ctx.bpf_buflen > kMaxBpfBufSize)
    {
        std::cout << "[DEBUG] qnx_raw_open: BPF buffer size " << g_qnx_ctx.bpf_buflen
                  << " exceeds maximum " << kMaxBpfBufSize << std::endl;
        close(fd);
        errno = ENOMEM;
        return -1;
    }

    g_qnx_ctx.bpf_buf_initialized = true;
    g_qnx_ctx.bpf_fd = fd;

    std::cout << "[DEBUG] qnx_raw_open: SUCCESS, fd=" << fd << ", ifname=" << g_qnx_ctx.iface_name << std::endl;
    return fd;
}

extern "C" int qnx_raw_recv(int fd, void *buf, int buf_len, timespec *hwts, int nonblock)
{
    if (fd < 0 || !buf || buf_len <= 0 || !hwts)
    {
        errno = EINVAL;
        return -1;
    }
    if (g_qnx_ctx.bpf_buflen == 0 || !g_qnx_ctx.bpf_buf_initialized)
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

    // bpf_buf is now a static array, no malloc needed

    for (;;)
    {
        if (g_qnx_ctx.bpf_off >= g_qnx_ctx.bpf_n)
        {
            ssize_t n = read(fd, g_qnx_ctx.bpf_buf, g_qnx_ctx.bpf_buflen);
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
            g_qnx_ctx.bpf_n = n;
            g_qnx_ctx.bpf_off = 0;
        }

        if (g_qnx_ctx.bpf_off + static_cast<ssize_t>(sizeof(bpf_hdr)) > g_qnx_ctx.bpf_n)
        {
            g_qnx_ctx.bpf_off = g_qnx_ctx.bpf_n;
            continue;
        }

        // Validate alignment before reinterpret_cast (safety-critical requirement)
        const auto ptr_value = reinterpret_cast<std::uintptr_t>(g_qnx_ctx.bpf_buf + g_qnx_ctx.bpf_off);
        if (ptr_value % alignof(bpf_hdr) != 0)
        {
            g_qnx_ctx.bpf_off = g_qnx_ctx.bpf_n;  // Skip to end
            continue;
        }

        auto *bh = reinterpret_cast<bpf_hdr *>(g_qnx_ctx.bpf_buf + g_qnx_ctx.bpf_off);

        // Check for integer overflow and bounds
        if (bh->bh_hdrlen > static_cast<u_int>(g_qnx_ctx.bpf_n) ||
            bh->bh_caplen > static_cast<u_int>(g_qnx_ctx.bpf_n) ||
            bh->bh_hdrlen + bh->bh_caplen < bh->bh_hdrlen ||  // Overflow check
            g_qnx_ctx.bpf_off + static_cast<ssize_t>(bh->bh_hdrlen) + static_cast<ssize_t>(bh->bh_caplen) > g_qnx_ctx.bpf_n)
        {
            g_qnx_ctx.bpf_off = g_qnx_ctx.bpf_n;
            continue;
        }

        unsigned char *pkt = reinterpret_cast<unsigned char *>(bh) + bh->bh_hdrlen;
        int caplen = static_cast<int>(bh->bh_caplen);

        ssize_t next_off = g_qnx_ctx.bpf_off + BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);

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
                    g_qnx_ctx.bpf_off = next_off;
                    continue;
                }
                // Use memcpy to avoid unaligned access
                uint16_t ethertype_vlan;
                std::memcpy(&ethertype_vlan, pkt + static_cast<int>(sizeof(::ethhdr)) + 2, sizeof(uint16_t));
                ethertype = ntohs(ethertype_vlan);
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
                if (get_hwts_tx_rx(g_qnx_ctx.iface_name, 1, ph, &ts) < 0)
                {
                    clock_gettime(CLOCK_REALTIME, &ts);
                }
                *hwts = ts;

                g_qnx_ctx.bpf_off = next_off;
                return frame_len;
            }
        }

        g_qnx_ctx.bpf_off = next_off;
    }
}

extern "C" int qnx_raw_send(int fd, const void *buf, int len, timespec *hwts)
{
    if (fd < 0 || !buf || len <= 0 || !hwts)
    {
        errno = EINVAL;
        return -1;
    }

    unsigned int frame_len = static_cast<unsigned int>(len);

    if (frame_len > 1500)
    {
        errno = EMSGSIZE;
        return -1;
    }

    std::memcpy(g_qnx_ctx.tx_frame, buf, frame_len);

    ssize_t n = write(fd, g_qnx_ctx.tx_frame, frame_len);
    if (n < 0)
    {
        return -1;
    }

    const auto *ph = reinterpret_cast<const PTPHeader *>(g_qnx_ctx.tx_frame + ETHER_HDR_LEN);
    timespec ts{};
    if (get_hwts_tx_rx(g_qnx_ctx.iface_name, 0, ph, &ts) < 0)
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
        strlcpy(g_qnx_ctx.iface_name, phc_dev, sizeof(g_qnx_ctx.iface_name));
    }
    return 0;
}

// QNX SDP 8.0: use io-sock/ptp.h API (struct ptp_time, PTP_GET/SET_TIME via ioctl).
extern "C" int qnx_phc_adjtime_step(int phc_fd, long long offset_ns)
{
    (void)phc_fd;

    if (offset_ns == 0)
        return 0;

    const int s = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        return -1;

    struct
    {
        struct ifdrv     ifd;
        struct ptp_time  tm;
    } cmd;

    std::memset(&cmd, 0, sizeof(cmd));
    std::strncpy(cmd.ifd.ifd_name, g_qnx_ctx.iface_name, sizeof(cmd.ifd.ifd_name) - 1);
    cmd.ifd.ifd_len  = sizeof(cmd.tm);
    cmd.ifd.ifd_data = &cmd.tm;

    cmd.ifd.ifd_cmd = PTP_GET_TIME;
    if (::ioctl(s, SIOCGDRVSPEC, &cmd) == -1)
    {
        ::close(s);
        return -1;
    }

    // ptp_time.sec is int64_t in QNX SDP 8.0
    const int64_t cur_ns = cmd.tm.sec * static_cast<int64_t>(NS_PER_SEC) +
                           static_cast<int64_t>(cmd.tm.nsec);
    const int64_t new_ns = cur_ns + offset_ns;

    cmd.tm.sec  = new_ns / static_cast<int64_t>(NS_PER_SEC);
    cmd.tm.nsec = static_cast<int32_t>(new_ns % static_cast<int64_t>(NS_PER_SEC));
    if (cmd.tm.nsec < 0)
    {
        cmd.tm.nsec += static_cast<int32_t>(NS_PER_SEC);
        cmd.tm.sec  -= 1;
    }

    cmd.ifd.ifd_cmd = PTP_SET_TIME;
    const int r = ::ioctl(s, SIOCSDRVSPEC, &cmd);
    ::close(s);
    return r;
}

extern "C" int qnx_phc_adjfreq_ppb(int phc_fd, long long freq_ppb)
{
    (void)phc_fd;
    (void)freq_ppb;
    return 0;
}
