/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

/**
 * @brief Minimal gPTP master for single-board PHC-slewing tests — QNX / EMAC.
 *
 * Sends Sync + FollowUp (two-step) at 1 Hz on a raw BPF socket.
 * The preciseOriginTimestamp in each FollowUp is taken from CLOCK_REALTIME
 * immediately after the corresponding Sync write.  The slave computes:
 *
 *   rate_ratio = ΔtPHC_rx / ΔtREALTIME_origin
 *
 * and slews its PHC toward CLOCK_REALTIME.
 *
 * Same-board single-interface usage:
 *   The slave's BPF fd must be opened with BIOCSSEESENT=1 so that it sees
 *   outgoing frames from this process without a physical loopback cable.
 *   (qnx_raw_shim.cpp is already patched to set BIOCSSEESENT=1.)
 *
 *   Terminal A: ./gptp_master  emac0
 *   Terminal B: ./timeslave_standalone emac0
 *
 * Cross-board usage (standard gPTP topology):
 *   Board 1 (master): ./gptp_master  emac0
 *   Board 2 (slave):  ./timeslave_standalone emac0
 *
 * Usage:
 *   ./gptp_master [interface]
 *   ./gptp_master emac0
 */

#include "score/TimeSlave/code/gptp/details/ptp_types.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <array>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace
{

volatile sig_atomic_t g_running = 1;  // NOLINT

void SigHandler(int /*sig*/) noexcept { g_running = 0; }

// IEEE 802.1AS / PTP layer-2 multicast destination MAC.
constexpr std::array<std::uint8_t, 6> kPtpMcastMac{0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};

// Minimal packed Ethernet header (avoid pulling in system ethhdr which may
// differ between QNX versions).
struct SCORE_TS_PACKED MasterEthHdr
{
    std::uint8_t  dst[6];
    std::uint8_t  src[6];
    std::uint16_t etype;
};

// ---------------------------------------------------------------------------
// Helper: write a network-byte-order Timestamp from a timespec.
// The parser (LoadTimestamp) applies ntohs/ntohl when reading, so we must
// write big-endian values here.
// ---------------------------------------------------------------------------
void EncodeNetTimestamp(score::ts::details::Timestamp& out,
                        const struct timespec& ts) noexcept
{
    const auto total_sec = static_cast<std::uint64_t>(ts.tv_sec);
    out.seconds_msb = htons(static_cast<std::uint16_t>((total_sec >> 32U) & 0xFFFFU));
    out.seconds_lsb = htonl(static_cast<std::uint32_t>(total_sec & 0xFFFFFFFFU));
    out.nanoseconds = htonl(static_cast<std::uint32_t>(ts.tv_nsec));
}

// ---------------------------------------------------------------------------
// Helper: derive EUI-64 ClockIdentity from a 6-byte MAC (EUI-48 → EUI-64).
// ---------------------------------------------------------------------------
score::ts::details::ClockIdentity MakeClock(
    const std::array<std::uint8_t, 6>& mac) noexcept
{
    score::ts::details::ClockIdentity id{};
    id.id[0] = mac[0];
    id.id[1] = mac[1];
    id.id[2] = mac[2];
    id.id[3] = 0xFFU;
    id.id[4] = 0xFEU;
    id.id[5] = mac[3];
    id.id[6] = mac[4];
    id.id[7] = mac[5];
    return id;
}

// ---------------------------------------------------------------------------
// GetMac: read the hardware MAC of a network interface via getifaddrs.
// ---------------------------------------------------------------------------
bool GetMac(const char* ifname,
            std::array<std::uint8_t, 6>& mac) noexcept
{
    struct ifaddrs* list = nullptr;
    if (::getifaddrs(&list) != 0)
        return false;

    bool found = false;
    for (const struct ifaddrs* ifa = list; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;
        if (ifa->ifa_addr->sa_family != AF_LINK)
            continue;
        if (std::strcmp(ifa->ifa_name, ifname) != 0)
            continue;
        const auto* dl =
            reinterpret_cast<const struct sockaddr_dl*>(ifa->ifa_addr);
        if (dl->sdl_alen >= static_cast<unsigned char>(mac.size()))
        {
            std::memcpy(mac.data(), LLADDR(dl), mac.size());
            found = true;
        }
        break;
    }
    ::freeifaddrs(list);
    return found;
}

// ---------------------------------------------------------------------------
// OpenBpf: open a BPF fd bound to the given interface for raw TX.
// ---------------------------------------------------------------------------
int OpenBpf(const char* ifname) noexcept
{
    char devpath[256]{};
    const char* sock_env = std::getenv("SOCK");
    if (sock_env != nullptr && sock_env[0] != '\0')
        std::snprintf(devpath, sizeof(devpath), "%s/dev/bpf0", sock_env);
    else
        std::snprintf(devpath, sizeof(devpath), "/dev/bpf");

    const int fd = ::open(devpath, O_RDWR);
    if (fd < 0)
    {
        std::perror("[gptp_master] open /dev/bpf");
        return -1;
    }

    ::ifreq ifr{};
    ::strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (::ioctl(fd, BIOCSETIF, &ifr) < 0)
    {
        std::perror("[gptp_master] BIOCSETIF");
        ::close(fd);
        return -1;
    }

    // Immediate mode: no read buffering delay (not required for TX-only, but
    // keeps the fd consistent if we ever add RX later).
    u_int one = 1U;
    (void)::ioctl(fd, BIOCIMMEDIATE, &one);

    return fd;
}

// ---------------------------------------------------------------------------
// SendSync: build and write a two-step Sync frame; record send time in ts_out.
// ---------------------------------------------------------------------------
void SendSync(int                                       bpf_fd,
              const std::array<std::uint8_t, 6>&        src_mac,
              const score::ts::details::ClockIdentity&  clock_id,
              std::uint16_t                             seqnum,
              struct timespec&                          ts_out) noexcept
{
    using namespace score::ts::details;

    // PTP payload
    SyncBody sync{};
    sync.ptpHdr.tsmt          = kPtpMsgtypeSync | kPtpTransportSpecific;
    sync.ptpHdr.version       = kPtpVersion;
    sync.ptpHdr.messageLength = htons(static_cast<std::uint16_t>(sizeof(SyncBody)));
    sync.ptpHdr.flagField[0]  = 0x02U;  // twoStepFlag (two-step clock)
    sync.ptpHdr.flagField[1]  = 0x00U;
    sync.ptpHdr.correctionField = 0;
    sync.ptpHdr.reserved2       = 0;
    sync.ptpHdr.sourcePortIdentity.clockIdentity = clock_id;
    sync.ptpHdr.sourcePortIdentity.portNumber    = htons(1U);
    sync.ptpHdr.sequenceId    = htons(seqnum);
    sync.ptpHdr.controlField  = static_cast<std::uint8_t>(ControlField::kSync);
    sync.ptpHdr.logMessageInterval = 0;  // 1 s interval (2^0)
    // originTimestamp = 0 (two-step; precise time comes in FollowUp)

    // Assemble Ethernet frame
    constexpr std::size_t kFrameLen = sizeof(MasterEthHdr) + sizeof(SyncBody);
    std::uint8_t frame[kFrameLen]{};
    auto* eth = reinterpret_cast<MasterEthHdr*>(frame);
    std::memcpy(eth->dst, kPtpMcastMac.data(), 6U);
    std::memcpy(eth->src, src_mac.data(), 6U);
    eth->etype = htons(0x88F7U);  // ETH_P_1588
    std::memcpy(frame + sizeof(MasterEthHdr), &sync, sizeof(SyncBody));

    // Send frame; record CLOCK_REALTIME after write as origin timestamp.
    // Post-send timestamp is closer to actual wire-egress time than pre-send.
    (void)::write(bpf_fd, frame, kFrameLen);
    (void)::clock_gettime(CLOCK_REALTIME, &ts_out);
}

// ---------------------------------------------------------------------------
// SendFollowUp: build and write a FollowUp carrying the Sync origin timestamp.
// ---------------------------------------------------------------------------
void SendFollowUp(int                                       bpf_fd,
                  const std::array<std::uint8_t, 6>&        src_mac,
                  const score::ts::details::ClockIdentity&  clock_id,
                  std::uint16_t                             seqnum,
                  const struct timespec&                    origin_ts) noexcept
{
    using namespace score::ts::details;

    FollowUpBody fup{};
    fup.ptpHdr.tsmt           = kPtpMsgtypeFollowUp | kPtpTransportSpecific;
    fup.ptpHdr.version        = kPtpVersion;
    fup.ptpHdr.messageLength  = htons(static_cast<std::uint16_t>(sizeof(FollowUpBody)));
    fup.ptpHdr.flagField[0]   = 0x00U;
    fup.ptpHdr.flagField[1]   = 0x00U;
    fup.ptpHdr.correctionField = 0;
    fup.ptpHdr.reserved2       = 0;
    fup.ptpHdr.sourcePortIdentity.clockIdentity = clock_id;
    fup.ptpHdr.sourcePortIdentity.portNumber    = htons(1U);
    fup.ptpHdr.sequenceId     = htons(seqnum);
    fup.ptpHdr.controlField   = static_cast<std::uint8_t>(ControlField::kFollowUp);
    fup.ptpHdr.logMessageInterval = 0;

    // Encode the Sync send time as the preciseOriginTimestamp (network byte order).
    EncodeNetTimestamp(fup.preciseOriginTimestamp, origin_ts);

    constexpr std::size_t kFrameLen = sizeof(MasterEthHdr) + sizeof(FollowUpBody);
    std::uint8_t frame[kFrameLen]{};
    auto* eth = reinterpret_cast<MasterEthHdr*>(frame);
    std::memcpy(eth->dst, kPtpMcastMac.data(), 6U);
    std::memcpy(eth->src, src_mac.data(), 6U);
    eth->etype = htons(0x88F7U);
    std::memcpy(frame + sizeof(MasterEthHdr), &fup, sizeof(FollowUpBody));

    (void)::write(bpf_fd, frame, kFrameLen);
}

}  // namespace

int main(int argc, char* argv[])
{
    std::signal(SIGINT,  SigHandler);
    std::signal(SIGTERM, SigHandler);

    const char* iface = (argc >= 2) ? argv[1] : "emac0";
    std::printf("[gptp_master] interface = %s\n", iface);

    // Resolve interface MAC address.
    std::array<std::uint8_t, 6> src_mac{};
    if (!GetMac(iface, src_mac))
    {
        std::fprintf(stderr, "[gptp_master] ERROR: cannot read MAC for %s\n", iface);
        return 1;
    }
    std::printf("[gptp_master] MAC = %02X:%02X:%02X:%02X:%02X:%02X\n",
                src_mac[0], src_mac[1], src_mac[2],
                src_mac[3], src_mac[4], src_mac[5]);

    const score::ts::details::ClockIdentity clock_id = MakeClock(src_mac);

    const int bpf_fd = OpenBpf(iface);
    if (bpf_fd < 0)
        return 1;

    std::printf("[gptp_master] Sending Sync+FUP every 1 s — Ctrl+C to stop\n");
    std::printf("[gptp_master] preciseOriginTimestamp source: CLOCK_REALTIME\n\n");

    std::uint16_t seqnum = 0U;

    // Anchor the send loop to a monotonic base so 1-second intervals are tight.
    struct timespec next{};
    (void)::clock_gettime(CLOCK_MONOTONIC, &next);

    while (g_running != 0)
    {
        // --- Send Sync then immediately FollowUp ---
        struct timespec origin_ts{};
        SendSync(bpf_fd, src_mac, clock_id, seqnum, origin_ts);
        SendFollowUp(bpf_fd, src_mac, clock_id, seqnum, origin_ts);

        std::printf("[%5u] Sync+FUP  origin=%lld.%09ld\n",
                    static_cast<unsigned>(seqnum),
                    static_cast<long long>(origin_ts.tv_sec),
                    origin_ts.tv_nsec);

        ++seqnum;  // uint16_t: wraps naturally at 0xFFFF

        // --- Sleep until next 1-second tick ---
        const std::int64_t next_ns =
            static_cast<std::int64_t>(next.tv_sec) * 1'000'000'000LL +
            next.tv_nsec + 1'000'000'000LL;
        next.tv_sec  = static_cast<time_t>(next_ns / 1'000'000'000LL);
        next.tv_nsec = static_cast<long>(next_ns % 1'000'000'000LL);
        (void)::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
    }

    std::printf("\n[gptp_master] Stopped after %u Sync pairs\n",
                static_cast<unsigned>(seqnum));
    ::close(bpf_fd);
    return 0;
}
