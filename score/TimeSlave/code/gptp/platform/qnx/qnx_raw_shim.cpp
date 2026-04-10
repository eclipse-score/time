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

// QNX BPF-based raw socket shim for gPTP frame capture and transmission.
// Provides qnx_raw_open / qnx_raw_recv / qnx_raw_send / qnx_phc_* symbols
// declared in raw_socket.cpp (extern "C").

#include <arpa/inet.h>
#include <fcntl.h>
#include <net/bpf.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// QNX SDP 8.0: PTP API constants (from io-sock/ptp.h, inlined to avoid
// struct PortIdentity redefinition conflict with details/ptp_types.h).
#define PTP_GET_TIME 0x102
#define PTP_SET_TIME 0x103
struct ptp_time
{
    int64_t sec;
    int32_t nsec;
};

// Inlined ptp_tstmp (from io-sock/ptp.h) — avoids PortIdentity name collision.
// A TX loopback frame contains an Ethernet header followed by this struct.
struct PtpTstmp
{
    uint32_t uid;
    ptp_time time;
};

// ── EtherType constants ───────────────────────────────────────────────────────
#ifndef ETH_P_8021Q
#define ETH_P_8021Q 0x8100U
#endif
#ifndef ETH_P_1588
#define ETH_P_1588 0x88F7U
#endif

// ── Self-contained ethernet header layout ────────────────────────────────────
struct GptpEthHdr
{
    unsigned char h_dest[6];
    unsigned char h_source[6];
    uint16_t h_proto;
};

static constexpr int64_t kNsPerSec = 1'000'000'000LL;
static constexpr std::size_t kMaxBpfBufSz = 65536U;
static constexpr int kMaxTxScanTries = 8;

// Caplen of a BPF TX loopback frame injected by the PTP driver:
//   Ethernet header (14 B) + ptp_tstmp payload (4 + 12 = 16 B) = 30 B
static constexpr int kTxLoopbackCaplen = static_cast<int>(sizeof(GptpEthHdr) + sizeof(PtpTstmp));

// ── BPF kernel filter: pass only IEEE 802.1AS (ETH_P_1588) frames ────────────
//   BPF_LD  H ABS 12   — load EtherType (bytes 12-13)
//   BPF_JEQ ETH_P_1588 — jump if match
//   BPF_RET (u_int)-1  — keep entire packet
//   BPF_RET 0          — drop
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static struct bpf_insn kPtp1588FilterInsns[] = {
    BPF_STMT(BPF_LD + BPF_H + BPF_ABS, 12),
    BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETH_P_1588, 0, 1),
    BPF_STMT(BPF_RET + BPF_K, static_cast<u_int>(-1)),
    BPF_STMT(BPF_RET + BPF_K, 0),
};
static const u_int kPtp1588FilterLen = static_cast<u_int>(sizeof(kPtp1588FilterInsns) / sizeof(kPtp1588FilterInsns[0]));

// ── Per-thread BPF context ───────────────────────────────────────────────────
struct QnxRawContext
{
    int bpf_fd = -1;
    u_int bpf_buflen = 0;
    char iface_name[IFNAMSIZ]{};
    unsigned char bpf_buf[kMaxBpfBufSz]{};
    ssize_t bpf_n = 0;
    ssize_t bpf_off = 0;
    bool initialized = false;
    unsigned char tx_frame[ETHER_HDR_LEN + 1500]{};

    // Secondary BPF fd with BIOCSSEESENT=1 for reading TX loopback timestamps.
    // Lazily opened on first qnx_raw_send() call.
    int tx_loopback_fd = -1;
    u_int tx_loopback_buflen = 0;
    unsigned char tx_loopback_buf[kMaxBpfBufSz]{};

    ~QnxRawContext()
    {
        if (bpf_fd >= 0)
        {
            ::close(bpf_fd);
            bpf_fd = -1;
        }
        if (tx_loopback_fd >= 0)
        {
            ::close(tx_loopback_fd);
            tx_loopback_fd = -1;
        }
    }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local QnxRawContext g_qnx_ctx;

// ── Internal helpers ──────────────────────────────────────────────────────────

// Convert a bpf_xhdr hardware timestamp to timespec.
// bpf_ts::bt_sec  — seconds (int64_t)
// bpf_ts::bt_frac — binary fraction of a second (uint64_t, unit = 2^-64 s)
// This is equivalent to bintime2timespec() from <sys/time.h>.
static void bpf_ts_to_timespec(const bpf_xhdr* bh, struct timespec* ts) noexcept
{
    ts->tv_sec = static_cast<time_t>(bh->bh_tstamp.bt_sec);
    const uint64_t top32 = bh->bh_tstamp.bt_frac >> 32U;
    ts->tv_nsec = static_cast<long>((top32 * 1'000'000'000ULL) >> 32U);
}

// Parse an Ethernet/VLAN frame; return byte offset of PTP payload or -1.
static int ptp_payload_offset(const unsigned char* frame, int caplen)
{
    if (caplen < static_cast<int>(sizeof(GptpEthHdr)))
        return -1;

    GptpEthHdr eth{};
    std::memcpy(&eth, frame, sizeof(GptpEthHdr));
    uint16_t etype = ntohs(eth.h_proto);
    int offset = static_cast<int>(sizeof(GptpEthHdr));

    if (etype == ETH_P_8021Q)
    {
        if (caplen < offset + 4)
            return -1;
        uint16_t inner{};
        std::memcpy(&inner, frame + offset + 2, sizeof(uint16_t));
        etype = ntohs(inner);
        offset += 4;
    }

    return (etype == ETH_P_1588) ? offset : -1;
}

// Open a secondary BPF fd on the same interface as main_fd, with
// BIOCSSEESENT=1 so our own TX frames appear as loopback records.
// Stores the resulting buffer length in g_qnx_ctx.tx_loopback_buflen.
// Returns the new fd or -1.
static int open_tx_loopback_fd(int main_fd) noexcept
{
    // Retrieve interface name from the already-bound main fd.
    ::ifreq ifr{};
    if (::ioctl(main_fd, BIOCGETIF, &ifr) < 0)
        return -1;

    char devpath[256]{};
    const char* sock_env = std::getenv("SOCK");
    if (sock_env != nullptr && sock_env[0] != '\0')
        std::snprintf(devpath, sizeof(devpath), "%s/dev/bpf0", sock_env);
    else
        std::snprintf(devpath, sizeof(devpath), "/dev/bpf");

    int lfd = ::open(devpath, O_RDWR);
    if (lfd < 0)
        return -1;

    if (::ioctl(lfd, BIOCSETIF, &ifr) < 0)
    {
        ::close(lfd);
        return -1;
    }

    // Enable loopback so our sent frames are visible on this fd.
    u_int one = 1U;
    (void)::ioctl(lfd, BIOCSSEESENT, &one);
    (void)::ioctl(lfd, BIOCIMMEDIATE, &one);

    // Request PTP hardware timestamps in bpf_xhdr format.
    u_int bpf_ts = BPF_T_PTP | BPF_T_BINTIME;
    (void)::ioctl(lfd, BIOCSTSTAMP, &bpf_ts);

    // Apply the same ETH_P_1588 kernel filter.
    struct bpf_program prog
    {
        kPtp1588FilterLen, kPtp1588FilterInsns
    };
    (void)::ioctl(lfd, BIOCSETF, &prog);

    u_int buflen = 0U;
    if (::ioctl(lfd, BIOCGBLEN, &buflen) < 0 || buflen == 0U || buflen > kMaxBpfBufSz)
    {
        ::close(lfd);
        return -1;
    }
    g_qnx_ctx.tx_loopback_buflen = buflen;
    return lfd;
}

// ── Public C interface ────────────────────────────────────────────────────────

extern "C" int qnx_raw_open(const char* ifname)
{
    if (ifname == nullptr)
    {
        errno = EINVAL;
        return -1;
    }

    ::strlcpy(g_qnx_ctx.iface_name, ifname, sizeof(g_qnx_ctx.iface_name));

    char devpath[256]{};
    const char* sock_env = std::getenv("SOCK");
    if (sock_env != nullptr && sock_env[0] != '\0')
        std::snprintf(devpath, sizeof(devpath), "%s/dev/bpf0", sock_env);
    else
        std::snprintf(devpath, sizeof(devpath), "/dev/bpf");

    int fd = ::open(devpath, O_RDWR);
    if (fd < 0)
        return -1;

    ::ifreq ifr{};
    ::strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (::ioctl(fd, BIOCSETIF, &ifr) < 0)
    {
        ::close(fd);
        return -1;
    }

    // Do NOT see our own TX frames on the main fd; use tx_loopback_fd instead.
    int zero = 0;
    (void)::ioctl(fd, BIOCSSEESENT, &zero);

    u_int yes = 1U;
    (void)::ioctl(fd, BIOCIMMEDIATE, &yes);
    (void)::ioctl(fd, BIOCPROMISC, &yes);

    // Request PTP hardware timestamps in bpf_xhdr format (IEEE 1588 clock).
    // Falls back gracefully: if unsupported, timestamps will be zero and
    // qnx_raw_recv() will fall back to CLOCK_REALTIME.
    u_int bpf_ts = BPF_T_PTP | BPF_T_BINTIME;
    (void)::ioctl(fd, BIOCSTSTAMP, &bpf_ts);

    // Install kernel BPF filter: discard all non-ETH_P_1588 frames early.
    struct bpf_program prog
    {
        kPtp1588FilterLen, kPtp1588FilterInsns
    };
    (void)::ioctl(fd, BIOCSETF, &prog);  // best-effort; userspace filter still runs

    if (::ioctl(fd, BIOCGBLEN, &g_qnx_ctx.bpf_buflen) < 0)
    {
        ::close(fd);
        return -1;
    }
    if (g_qnx_ctx.bpf_buflen > kMaxBpfBufSz)
    {
        ::close(fd);
        errno = ENOMEM;
        return -1;
    }

    g_qnx_ctx.bpf_fd = fd;
    g_qnx_ctx.initialized = true;
    return fd;
}

extern "C" int qnx_raw_recv(int fd, void* buf, int buf_len, timespec* hwts, int nonblock)
{
    if (fd < 0 || buf == nullptr || buf_len <= 0 || hwts == nullptr)
    {
        errno = EINVAL;
        return -1;
    }
    if (!g_qnx_ctx.initialized || g_qnx_ctx.bpf_buflen == 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (nonblock != 0)
    {
        int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags >= 0)
            (void)::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    for (;;)
    {
        // Refill BPF read buffer when exhausted.
        if (g_qnx_ctx.bpf_off >= g_qnx_ctx.bpf_n)
        {
            ssize_t n = ::read(fd, g_qnx_ctx.bpf_buf, g_qnx_ctx.bpf_buflen);
            if (n < 0)
                return -1;
            if (n == 0)
            {
                if (nonblock != 0)
                {
                    errno = EAGAIN;
                    return -1;
                }
                continue;
            }
            g_qnx_ctx.bpf_n = n;
            g_qnx_ctx.bpf_off = 0;
        }

        // Need at least sizeof(bpf_xhdr) bytes for the header.
        if (g_qnx_ctx.bpf_off + static_cast<ssize_t>(sizeof(bpf_xhdr)) > g_qnx_ctx.bpf_n)
        {
            g_qnx_ctx.bpf_off = g_qnx_ctx.bpf_n;
            continue;
        }

        // Verify 8-byte alignment required by bpf_xhdr.
        const auto ptr_val = reinterpret_cast<std::uintptr_t>(g_qnx_ctx.bpf_buf + g_qnx_ctx.bpf_off);
        if (ptr_val % alignof(bpf_xhdr) != 0U)
        {
            g_qnx_ctx.bpf_off = g_qnx_ctx.bpf_n;
            continue;
        }

        const auto* bh = reinterpret_cast<const bpf_xhdr*>(g_qnx_ctx.bpf_buf + g_qnx_ctx.bpf_off);

        // Bounds checks.
        if (bh->bh_hdrlen < static_cast<u_short>(sizeof(bpf_xhdr)) ||
            bh->bh_caplen > static_cast<bpf_u_int32>(g_qnx_ctx.bpf_n) ||
            g_qnx_ctx.bpf_off + static_cast<ssize_t>(bh->bh_hdrlen) + static_cast<ssize_t>(bh->bh_caplen) >
                g_qnx_ctx.bpf_n)
        {
            g_qnx_ctx.bpf_off = g_qnx_ctx.bpf_n;
            continue;
        }

        const unsigned char* pkt = reinterpret_cast<const unsigned char*>(bh) + bh->bh_hdrlen;
        const int caplen = static_cast<int>(bh->bh_caplen);
        const ssize_t next_off = g_qnx_ctx.bpf_off + static_cast<ssize_t>(BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen));

        // Skip TX loopback frames (BIOCSSEESENT=0 should prevent them on the
        // main fd, but guard defensively: a loopback frame has a fixed small
        // caplen equal to ETH header + ptp_tstmp, not a valid PTP message).
        if (caplen == kTxLoopbackCaplen)
        {
            g_qnx_ctx.bpf_off = next_off;
            continue;
        }

        const int ptp_off = ptp_payload_offset(pkt, caplen);
        if (ptp_off >= 0)
        {
            // Use PTP hardware RX timestamp from bpf_xhdr.
            // bt_sec==0 && bt_frac==0 means the driver did not provide a HW
            // timestamp; fall back to CLOCK_REALTIME in that case.
            if (bh->bh_tstamp.bt_sec != 0 || bh->bh_tstamp.bt_frac != 0)
            {
                bpf_ts_to_timespec(bh, hwts);
            }
            else
            {
                (void)::clock_gettime(CLOCK_REALTIME, hwts);
            }

            const int frame_len = std::min(caplen, buf_len);
            std::memcpy(buf, pkt, static_cast<std::size_t>(frame_len));
            g_qnx_ctx.bpf_off = next_off;
            return frame_len;
        }

        g_qnx_ctx.bpf_off = next_off;
    }
}

extern "C" int qnx_raw_send(int fd, const void* buf, int len, timespec* hwts)
{
    if (fd < 0 || buf == nullptr || len <= 0 || hwts == nullptr)
    {
        errno = EINVAL;
        return -1;
    }
    if (static_cast<unsigned int>(len) > 1500U)
    {
        errno = EMSGSIZE;
        return -1;
    }

    std::memcpy(g_qnx_ctx.tx_frame, buf, static_cast<std::size_t>(len));
    ssize_t n = ::write(fd, g_qnx_ctx.tx_frame, static_cast<std::size_t>(len));
    if (n < 0)
        return -1;

    // Attempt to obtain a hardware TX timestamp via the BPF loopback mechanism:
    //   1. BIOCGTSTAMPID returns the UID assigned to the just-sent frame.
    //   2. The driver inserts a loopback record on fds with BIOCSSEESENT=1;
    //      its payload is a ptp_tstmp struct carrying the actual HW timestamp.
    //   3. We scan the secondary loopback fd for a record whose uid matches.
    // If any step fails, we fall back to a CLOCK_REALTIME software timestamp.
    uint32_t tx_uid = 0U;
    if (::ioctl(fd, BIOCGTSTAMPID, &tx_uid) == 0)
    {
        // Lazy-open the secondary fd (needs BIOCGETIF to recover iface name).
        if (g_qnx_ctx.tx_loopback_fd < 0)
            g_qnx_ctx.tx_loopback_fd = open_tx_loopback_fd(fd);

        if (g_qnx_ctx.tx_loopback_fd >= 0 && g_qnx_ctx.tx_loopback_buflen > 0)
        {
            const int lfd = g_qnx_ctx.tx_loopback_fd;

            // Non-blocking scan: the loopback frame typically arrives within
            // a few microseconds; we try kMaxTxScanTries reads.
            int flags = ::fcntl(lfd, F_GETFL, 0);
            (void)::fcntl(lfd, F_SETFL, (flags >= 0 ? flags : 0) | O_NONBLOCK);

            for (int tries = 0; tries < kMaxTxScanTries; ++tries)
            {
                ssize_t nr = ::read(lfd, g_qnx_ctx.tx_loopback_buf, g_qnx_ctx.tx_loopback_buflen);
                if (nr <= 0)
                    break;

                ssize_t off = 0;
                while (off + static_cast<ssize_t>(sizeof(bpf_xhdr)) <= nr)
                {
                    const auto pv = reinterpret_cast<std::uintptr_t>(g_qnx_ctx.tx_loopback_buf + off);
                    if (pv % alignof(bpf_xhdr) != 0U)
                        break;

                    const auto* bh = reinterpret_cast<const bpf_xhdr*>(g_qnx_ctx.tx_loopback_buf + off);

                    if (bh->bh_hdrlen < static_cast<u_short>(sizeof(bpf_xhdr)) ||
                        off + static_cast<ssize_t>(bh->bh_hdrlen) + static_cast<ssize_t>(bh->bh_caplen) > nr)
                        break;

                    const unsigned char* pkt = reinterpret_cast<const unsigned char*>(bh) + bh->bh_hdrlen;
                    const int caplen = static_cast<int>(bh->bh_caplen);
                    const ssize_t next = off + static_cast<ssize_t>(BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen));

                    // A TX loopback record has a fixed caplen and contains a
                    // ptp_tstmp payload right after the Ethernet header.
                    if (caplen == kTxLoopbackCaplen)
                    {
                        const auto* tstmp = reinterpret_cast<const PtpTstmp*>(pkt + sizeof(GptpEthHdr));
                        if (tstmp->uid == tx_uid)
                        {
                            hwts->tv_sec = static_cast<time_t>(tstmp->time.sec);
                            hwts->tv_nsec = static_cast<long>(tstmp->time.nsec);
                            return static_cast<int>(len);
                        }
                    }
                    off = next;
                }
            }
        }
    }

    // Fallback: software TX timestamp.
    (void)::clock_gettime(CLOCK_REALTIME, hwts);
    return static_cast<int>(len);
}

// ── PHC clock adjustment (QNX SDP 8.0 io-sock/ptp.h ioctl path) ──────────────

extern "C" int qnx_phc_open(const char* phc_dev)
{
    if (phc_dev != nullptr && phc_dev[0] != '\0' && phc_dev[0] != '/')
        ::strlcpy(g_qnx_ctx.iface_name, phc_dev, sizeof(g_qnx_ctx.iface_name));
    return 0;
}

extern "C" int qnx_phc_adjtime_step(int /*phc_fd*/, long long offset_ns)
{
    if (offset_ns == 0)
        return 0;

    const int s = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        return -1;

    struct
    {
        struct ifdrv ifd;
        struct ptp_time tm;
    } cmd{};

    std::strncpy(cmd.ifd.ifd_name, g_qnx_ctx.iface_name, sizeof(cmd.ifd.ifd_name) - 1U);
    cmd.ifd.ifd_len = sizeof(cmd.tm);
    cmd.ifd.ifd_data = &cmd.tm;
    cmd.ifd.ifd_cmd = PTP_GET_TIME;

    if (::ioctl(s, SIOCGDRVSPEC, &cmd) == -1)
    {
        ::close(s);
        return -1;
    }

    const int64_t cur_ns = cmd.tm.sec * kNsPerSec + static_cast<int64_t>(cmd.tm.nsec);
    const int64_t new_ns = cur_ns + static_cast<int64_t>(offset_ns);

    cmd.tm.sec = new_ns / kNsPerSec;
    cmd.tm.nsec = static_cast<int32_t>(new_ns % kNsPerSec);
    if (cmd.tm.nsec < 0)
    {
        cmd.tm.nsec += static_cast<int32_t>(kNsPerSec);
        cmd.tm.sec -= 1;
    }

    cmd.ifd.ifd_cmd = PTP_SET_TIME;
    const int r = ::ioctl(s, SIOCSDRVSPEC, &cmd);
    ::close(s);
    return r;
}

extern "C" int qnx_phc_adjfreq_ppb(int /*phc_fd*/, long long freq_ppb)
{
    if (freq_ppb == 0)
        return 0;

    const int s = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        return -1;

    // Convert ppb to ppm (EMAC_PTP_ADJ_FREQ_PPM expects ppm)
    int ppm = static_cast<int>(freq_ppb / 1000LL);

    struct
    {
        struct ifdrv ifd;
        int adj_ppm;
    } cmd{};

    std::strncpy(cmd.ifd.ifd_name, g_qnx_ctx.iface_name, sizeof(cmd.ifd.ifd_name) - 1U);
    cmd.ifd.ifd_len = sizeof(cmd.adj_ppm);
    cmd.ifd.ifd_data = &cmd.adj_ppm;
    cmd.ifd.ifd_cmd = 0x200;  // EMAC_PTP_ADJ_FREQ_PPM
    cmd.adj_ppm = ppm;

    const int r = ::ioctl(s, SIOCGDRVSPEC, &cmd);
    ::close(s);
    return r;
}
