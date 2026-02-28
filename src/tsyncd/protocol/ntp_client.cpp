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
#include "ntp_client.hpp"
#include "score_time/utils/time_utils.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

#include <cerrno>
#include <ctime>
#include <cstring>
#include <cmath>

namespace tsyncd::ntp
{
    namespace
    {
        static constexpr std::uint64_t kUnixToNtpSeconds = 2208988800ULL; // 1970->1900

#pragma pack(push, 1)
        struct NtpPacket
        {
            std::uint8_t li_vn_mode;
            std::uint8_t stratum;
            std::uint8_t poll;
            std::int8_t precision;
            std::uint32_t root_delay;
            std::uint32_t root_dispersion;
            std::uint32_t ref_id;
            std::uint64_t ref_ts;
            std::uint64_t orig_ts;
            std::uint64_t recv_ts;
            std::uint64_t tx_ts;
        };
#pragma pack(pop)

        inline std::uint64_t HostToNtp(std::int64_t unix_ns)
        {
            const std::int64_t unix_s = unix_ns / 1'000'000'000LL;
            const std::int64_t unix_rem_ns = unix_ns - unix_s * 1'000'000'000LL;
            const std::uint64_t ntp_s = static_cast<std::uint64_t>(unix_s) + kUnixToNtpSeconds;
            const std::uint64_t frac = (static_cast<std::uint64_t>(unix_rem_ns) << 32) / 1'000'000'000ULL;
            return (ntp_s << 32) | (frac & 0xFFFFFFFFULL);
        }

        inline std::int64_t NtpToUnixNs(std::uint64_t ntp_ts_be)
        {
            const std::uint64_t ntp_ts = be64toh(ntp_ts_be);
            const std::uint64_t ntp_s = ntp_ts >> 32;
            const std::uint64_t frac = ntp_ts & 0xFFFFFFFFULL;
            if (ntp_s < kUnixToNtpSeconds)
                return 0;
            const std::uint64_t unix_s = ntp_s - kUnixToNtpSeconds;
            const std::uint64_t unix_ns = unix_s * 1'000'000'000ULL + (frac * 1'000'000'000ULL >> 32);
            return static_cast<std::int64_t>(unix_ns);
        }

        inline bool Resolve(const std::string &host, int port, ::sockaddr_storage &out, socklen_t &outlen)
        {
            ::addrinfo hints{};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;

            ::addrinfo *res = nullptr;
            const std::string sport = std::to_string(port);
            if (::getaddrinfo(host.c_str(), sport.c_str(), &hints, &res) != 0 || !res)
                return false;

            for (::addrinfo *p = res; p; p = p->ai_next)
            {
                if (!p->ai_addr || p->ai_addrlen <= 0)
                    continue;
                if (static_cast<std::size_t>(p->ai_addrlen) > sizeof(out))
                    continue;
                std::memcpy(&out, p->ai_addr, static_cast<std::size_t>(p->ai_addrlen));
                outlen = static_cast<socklen_t>(p->ai_addrlen);
                ::freeaddrinfo(res);
                return true;
            }
            ::freeaddrinfo(res);
            return false;
        }

    } // namespace

    Client::Client(Options opt) : opt_(std::move(opt)) {}

    std::optional<Sample> Client::QueryOnce() const
    {
        for (const auto &host : opt_.servers)
        {
            ::sockaddr_storage addr{};
            socklen_t addrlen = 0;
            if (!Resolve(host, opt_.port, addr, addrlen))
                continue;

            const int fd = ::socket(addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
            if (fd < 0)
                continue;

            ::timeval tv{};
            tv.tv_sec = opt_.timeout_ms / 1000;
            tv.tv_usec = (opt_.timeout_ms % 1000) * 1000;
            (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            NtpPacket req{};
            req.li_vn_mode = static_cast<std::uint8_t>((0 << 6) | (4 << 3) | 3); // LI=0, VN=4, MODE=3(client)

            const std::int64_t t1 = score_time::utils::ClockNs(CLOCK_REALTIME);
            req.tx_ts = htobe64(HostToNtp(t1));

            const ssize_t sent = ::sendto(fd, &req, sizeof(req), 0, reinterpret_cast<const ::sockaddr *>(&addr), addrlen);
            if (sent != static_cast<ssize_t>(sizeof(req)))
            {
                ::close(fd);
                continue;
            }

            NtpPacket resp{};
            ::sockaddr_storage peer{};
            socklen_t peerlen = sizeof(peer);
            const ssize_t recvd = ::recvfrom(fd, &resp, sizeof(resp), 0, reinterpret_cast<::sockaddr *>(&peer), &peerlen);
            const std::int64_t t4 = score_time::utils::ClockNs(CLOCK_REALTIME);
            const std::int64_t mono_rx = score_time::utils::ClockNs(CLOCK_MONOTONIC);
            ::close(fd);
            if (recvd < static_cast<ssize_t>(sizeof(resp)))
                continue;

            const std::int64_t t2 = NtpToUnixNs(resp.recv_ts);
            const std::int64_t t3 = NtpToUnixNs(resp.tx_ts);
            if (t2 <= 0 || t3 <= 0)
                continue;

            // NTP formulas. Offset is server - local.
            const std::int64_t offset = ((t2 - t1) + (t3 - t4)) / 2;
            const std::int64_t delay = (t4 - t1) - (t3 - t2);
            if (delay < 0)
                continue;

            Sample s{};
            s.offset_ns = offset;
            s.delay_ns = delay;
            s.mono_rx_ns = mono_rx;
            s.local_tx_ns = t1;
            s.local_rx_ns = t4;
            s.server_rx_ns = t2;
            s.server_tx_ns = t3;
            return s;
        }
        return std::nullopt;
    }

    Estimator::Estimator(Options opt) : opt_(std::move(opt)) {}

    void Estimator::Update(const Sample &s)
    {
        if (s.delay_ns <= 0 || s.delay_ns > opt_.max_reasonable_delay_ns)
        {
            est_.bad_samples++;
            return;
        }

        // Jitter proxy: half of delay + absolute change in offset
        const std::int64_t new_jitter = (s.delay_ns / 2) + std::llabs(s.offset_ns - est_.offset_ns);

        if (est_.good_samples == 0)
        {
            est_.offset_ns = s.offset_ns;
            est_.jitter_ns = new_jitter;
        }
        else
        {
            est_.offset_ns = static_cast<std::int64_t>(
                (1.0 - opt_.offset_ewma_alpha) * static_cast<double>(est_.offset_ns) +
                opt_.offset_ewma_alpha * static_cast<double>(s.offset_ns));
            est_.jitter_ns = static_cast<std::int64_t>(
                (1.0 - opt_.jitter_ewma_alpha) * static_cast<double>(est_.jitter_ns) +
                opt_.jitter_ewma_alpha * static_cast<double>(new_jitter));
        }

        // Inaccuracy: at least half of delay, plus some jitter margin.
        est_.inaccuracy_ns = std::max<std::int64_t>(s.delay_ns / 2, est_.jitter_ns);

        est_.good_samples++;
        est_.last_update_mono_ns = s.mono_rx_ns;
        if (est_.good_samples >= static_cast<std::uint32_t>(opt_.samples_to_lock))
        {
            est_.locked = true;
        }
    }

    void Estimator::MarkTimeout(std::int64_t mono_now_ns)
    {
        est_.bad_samples++;
        // If too long without updates, drop lock.
        if (est_.last_update_mono_ns != 0 && (mono_now_ns - est_.last_update_mono_ns) > 5'000'000'000LL)
        { // 5s
            est_.locked = false;
            est_.good_samples = 0;
        }
    }

    Estimate Estimator::Snapshot() const { return est_; }

} // namespace tsyncd::ntp
