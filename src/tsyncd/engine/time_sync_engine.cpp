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
#include "time_sync_engine.hpp"

#include "eth_protocol.hpp"
#include "gptp_protocol.hpp"
#include "raw_socket.hpp"
#include "net_identity.hpp"
#include "score_time/utils/pthread_lock_guard.hpp"
#include "score_time/utils/time_utils.hpp"

#include <iostream>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef _QNX_PLAT
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <sys/timex.h>
#else
#include "qnx_raw_shim.hpp"
#endif

namespace tsyncd
{

    namespace
    {

        static void *rx_entry(void *arg)
        {
            if (!arg)
            {
                return nullptr;  // Defensive: should never happen in correct usage
            }
            reinterpret_cast<TimeSyncEngine *>(arg)->RxLoop();
            return nullptr;
        }
        static void *pdelay_entry(void *arg)
        {
            if (!arg)
            {
                return nullptr;  // Defensive: should never happen in correct usage
            }
            reinterpret_cast<TimeSyncEngine *>(arg)->PdelayLoop();
            return nullptr;
        }
        static void *abs_entry(void *arg)
        {
            if (!arg)
            {
                return nullptr;  // Defensive: should never happen in correct usage
            }
            reinterpret_cast<TimeSyncEngine *>(arg)->AbsLoop();
            return nullptr;
        }

        struct __attribute__((packed)) ExternalAbsMsg
        {
            std::uint32_t magic;
            std::uint16_t version;
            std::uint16_t reserved;
            std::int64_t utc_ns;
            std::int64_t inaccuracy_ns;
            std::uint8_t sec_qual;
            std::uint8_t reserved2[7];
        };

        constexpr std::uint32_t kAbsMsgMagic = 0x54494D45U;
        constexpr std::uint16_t kAbsMsgVer = 1;

    } // namespace

    TimeSyncEngine::TimeSyncEngine(const EngineOptions &opt) : opt_(opt), ctx_{}
    {
        ctx_.raw_fd = -1;
        ctx_.phc_fd = -1;
        ctx_.state = TsyncState::kEmpty;

        // Initialize mutex with error checking
        const int mutex_ret = pthread_mutex_init(&ctx_.pdelay_lock, nullptr);
        if (mutex_ret != 0)
        {
            std::fprintf(stderr, "tsyncd: pthread_mutex_init failed: %s (errno=%d)\n",
                         std::strerror(mutex_ret), mutex_ret);
            std::abort();  // Critical failure in safety-critical system
        }

        ntp::Client::Options copt;
        copt.servers = opt_.ntp_servers;
        copt.port = opt_.ntp_port;
        copt.timeout_ms = opt_.ntp_request_timeout_ms;
        ntp_client_ = ntp::Client(copt);

        ntp::Estimator::Options eopt;
        eopt.samples_to_lock = opt_.ntp_samples_to_lock;
        eopt.offset_ewma_alpha = opt_.ntp_offset_ewma_alpha;
        eopt.jitter_ewma_alpha = opt_.ntp_jitter_ewma_alpha;
        ntp_estimator_ = ntp::Estimator(eopt);
    }

    TimeSyncEngine::~TimeSyncEngine()
    {
        Stop();
        pthread_mutex_destroy(&ctx_.pdelay_lock);
    }

    bool TimeSyncEngine::Start()
    {
        std::cout << "[DEBUG] Start() called" << std::endl;

        // Initialize stop flag FIRST, before any initialization that might check it
        stop_.store(false, std::memory_order_seq_cst);

        if (!InitShm())
        {
            std::cout << "[DEBUG] InitShm() FAILED" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] InitShm() OK" << std::endl;

        if (!InitPhc())
        {
            std::cout << "[DEBUG] InitPhc() FAILED" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] InitPhc() OK" << std::endl;

        if (!InitRawSocket())
        {
            std::cout << "[DEBUG] InitRawSocket() FAILED" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] InitRawSocket() OK" << std::endl;

        if (!InitHwTimestamping())
        {
            std::cout << "[DEBUG] InitHwTimestamping() FAILED" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] InitHwTimestamping() OK" << std::endl;

        (void)InitAbsSourceSocket();
        std::cout << "[DEBUG] InitAbsSourceSocket() OK (optional)" << std::endl;

        if (pthread_create(&rx_th_, nullptr, &rx_entry, this) != 0)
        {
            std::cout << "[DEBUG] RX thread creation FAILED" << std::endl;
            return false;
        }
        rx_started_ = true;
        std::cout << "[DEBUG] RX thread created OK" << std::endl;

        if (pthread_create(&pdelay_th_, nullptr, &pdelay_entry, this) != 0)
        {
            std::cout << "[DEBUG] PDelay thread creation FAILED" << std::endl;
            Stop();
            return false;
        }
        pdelay_started_ = true;
        std::cout << "[DEBUG] PDelay thread created OK" << std::endl;

        if (pthread_create(&abs_th_, nullptr, &abs_entry, this) != 0)
        {
            std::cout << "[DEBUG] Abs thread creation FAILED" << std::endl;
            Stop();
            return false;
        }
        abs_started_ = true;
        std::cout << "[DEBUG] Abs thread created OK" << std::endl;

        std::cout << "[DEBUG] Start() completed successfully" << std::endl;
        return true;
    }

    void TimeSyncEngine::Stop()
    {
        stop_.store(true, std::memory_order_seq_cst);

        if (rx_started_)
        {
            pthread_join(rx_th_, nullptr);
            rx_started_ = false;
        }
        if (pdelay_started_)
        {
            pthread_join(pdelay_th_, nullptr);
            pdelay_started_ = false;
        }
        if (abs_started_)
        {
            pthread_join(abs_th_, nullptr);
            abs_started_ = false;
        }

        if (abs_sock_fd_ >= 0)
        {
            ::close(abs_sock_fd_);
            abs_sock_fd_ = -1;
        }
        if (ctx_.raw_fd >= 0)
        {
            ::close(ctx_.raw_fd);
            ctx_.raw_fd = -1;
        }
        if (ctx_.phc_fd >= 0)
        {
            ::close(ctx_.phc_fd);
            ctx_.phc_fd = -1;
        }
        shared_ = nullptr;
        shm_.Close();
    }

    bool TimeSyncEngine::InitShm()
    {
        const std::size_t need = sizeof(score_time::ipc::SharedState);
        const std::size_t size = (opt_.shm_size < need) ? need : opt_.shm_size;
        if (!shm_.Open(opt_.shm_name, size, /*create_or_open=*/true))
        {
            std::fprintf(stderr, "tsyncd: shm open failed (%s)\n", opt_.shm_name.c_str());
            return false;
        }

        void *raw_addr = shm_.Addr();
        const std::size_t shm_size = shm_.Size();

        // Validate alignment
        if (reinterpret_cast<std::uintptr_t>(raw_addr) % alignof(score_time::ipc::SharedState) != 0)
        {
            std::fprintf(stderr, "tsyncd: shared memory not aligned\n");
            return false;
        }

        // Validate size
        if (shm_size < sizeof(score_time::ipc::SharedState))
        {
            std::fprintf(stderr, "tsyncd: shared memory too small: %zu < %zu\n",
                         shm_size, sizeof(score_time::ipc::SharedState));
            return false;
        }

        shared_ = reinterpret_cast<score_time::ipc::SharedState *>(raw_addr);

        const auto magic = shared_->magic.load(std::memory_order_acquire);
        const auto ver = shared_->version.load(std::memory_order_acquire);
        const auto struct_size = shared_->struct_size.load(std::memory_order_acquire);
        if (magic != score_time::ipc::SharedState::kMagic ||
            ver != score_time::ipc::SharedState::kVersion ||
            struct_size != sizeof(score_time::ipc::SharedState))
        {
            std::memset(shared_, 0, sizeof(*shared_));
            shared_->magic.store(score_time::ipc::SharedState::kMagic, std::memory_order_release);
            shared_->version.store(score_time::ipc::SharedState::kVersion, std::memory_order_release);
            shared_->struct_size.store(static_cast<std::uint32_t>(sizeof(score_time::ipc::SharedState)), std::memory_order_release);
            shared_->vehicle_acc.store(score_time::AccuracyQualifier::kNoTime, std::memory_order_release);
            shared_->vehicle_tpq.store(score_time::TimePointQualifier::kQM, std::memory_order_release);
            shared_->abs_acc.store(score_time::AbsoluteAccuracyQualifier::kInaccuracyNotAvailable, std::memory_order_release);
            shared_->abs_sec.store(score_time::AbsoluteSecurityQualifier::kNoTimeAvailable, std::memory_order_release);
        }
        return true;
    }

    bool TimeSyncEngine::InitAbsSourceSocket()
    {
        if (!opt_.abs_external_enable)
            return false;
        if (opt_.abs_source_socket.empty())
            return false;

        const int fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd < 0)
            return false;

        ::sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", opt_.abs_source_socket.c_str());

        ::unlink(addr.sun_path);
        if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        {
            ::close(fd);
            return false;
        }

        const int flags = ::fcntl(fd, F_GETFL, 0);
        ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        abs_sock_fd_ = fd;
        return true;
    }

    bool TimeSyncEngine::InitPhc()
    {
#ifdef _QNX_PLAT
        ctx_.phc_fd = qnx_phc_open(opt_.phc_device.c_str());
        ctx_.clk_id = CLOCK_MONOTONIC;
#else
        ctx_.phc_fd = ::open(opt_.phc_device.c_str(), O_RDWR);
        while (ctx_.phc_fd < 0 && !stop_.load(std::memory_order_seq_cst))
        {
            std::fprintf(stderr, "tsyncd: failed to open PHC %s (%d), retry...\n", opt_.phc_device.c_str(), errno);
            ::sleep(1);
            ctx_.phc_fd = ::open(opt_.phc_device.c_str(), O_RDWR);
        }
        if (ctx_.phc_fd < 0)
            return false;

        ctx_.clk_id = (~(clockid_t)(ctx_.phc_fd) << 3) | 3;
#endif
        return InitClockIdentity();
    }

    bool TimeSyncEngine::InitRawSocket()
    {
        return setup_raw_socket(ctx_.raw_fd, opt_.iface_name.c_str()) == 0;
    }

    bool TimeSyncEngine::InitHwTimestamping()
    {
#ifdef _QNX_PLAT
        return true;
#else
        struct ifreq ifr{};
        struct hwtstamp_config cfg{};
        std::snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", opt_.iface_name.c_str());
        ifr.ifr_data = reinterpret_cast<char *>(&cfg);

        cfg.tx_type = HWTSTAMP_TX_ON;
        cfg.rx_filter = HWTSTAMP_FILTER_ALL;

        if (::ioctl(ctx_.raw_fd, SIOCSHWTSTAMP, &ifr) < 0)
        {
            cfg.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;
            (void)::ioctl(ctx_.raw_fd, SIOCSHWTSTAMP, &ifr);
        }

        int ts_opts = SOF_TIMESTAMPING_TX_HARDWARE |
                      SOF_TIMESTAMPING_RX_HARDWARE |
                      SOF_TIMESTAMPING_RAW_HARDWARE;
        if (::setsockopt(ctx_.raw_fd, SOL_SOCKET, SO_TIMESTAMPING, &ts_opts, sizeof(ts_opts)) < 0)
        {
            std::fprintf(stderr, "tsyncd: setsockopt(SO_TIMESTAMPING) failed (%d)\n", errno);
            return false;
        }
        return true;
#endif
    }

    bool TimeSyncEngine::InitClockIdentity()
    {
        return generate_clock_identity(ctx_.clockIdentity, opt_.iface_name.c_str()) == 0;
    }

    void TimeSyncEngine::RxLoop()
    {
        ::timespec hwts{};

        while (!stop_.load(std::memory_order_seq_cst))
        {
            const int n = raw_recvMsg(ctx_.raw_fd, rx_buffer_.data(), &hwts, /*flag=*/0);
            if (n <= 0)
                continue;
            HandlePacket(rx_buffer_.data(), n, hwts);
        }
    }

    void TimeSyncEngine::PdelayLoop()
    {
        ::timespec next{};
        ::clock_gettime(CLOCK_MONOTONIC, &next);

        next.tv_sec += 2;

        const std::int64_t interval_ns =
            (opt_.pdelay_req_interval_ms > 0
                 ? static_cast<std::int64_t>(opt_.pdelay_req_interval_ms) * 1'000'000LL
                 : 1'000'000'000LL);

        while (!stop_.load(std::memory_order_seq_cst))
        {
            ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

            if (stop_.load(std::memory_order_seq_cst))
                break;

            {
                score_time::utils::PthreadLockGuard lock(&ctx_.pdelay_lock);
                (void)SendPdelayRequest();
            }

            // Calculate next wake time
            const std::int64_t next_ns = timespec_to_tmv(next).ns + interval_ns;
            next.tv_sec = static_cast<time_t>(next_ns / 1'000'000'000LL);
            next.tv_nsec = static_cast<long>(next_ns % 1'000'000'000LL);
        }
    }

    void TimeSyncEngine::AbsLoop()
    {
        std::int64_t last_state_hash = 0;

        ::timespec next{};
        ::clock_gettime(CLOCK_MONOTONIC, &next);
        const std::int64_t interval_ns = static_cast<std::int64_t>(opt_.abs_publish_interval_ms) * 1'000'000LL;

        while (!stop_.load(std::memory_order_seq_cst))
        {
            const std::int64_t mono_now = score_time::utils::ClockNs(CLOCK_MONOTONIC);
            if (opt_.sync_timeout_ms > 0)
            {
                const std::int64_t last_sync = last_sync_event_mono_ns_.load(std::memory_order_acquire);
                if (last_sync > 0)
                {
                    const std::int64_t delta = mono_now - last_sync;
                    const std::int64_t timeout_ns = static_cast<std::int64_t>(opt_.sync_timeout_ms) * 1'000'000LL;
                    if (delta > timeout_ns)
                    {
                        if (!sync_timeout_logged_.load(std::memory_order_acquire))
                        {
                            std::cout << "[WARN] Sync timeout: last Sync/Fup older than "
                                      << (delta / 1'000'000LL) << " ms" << std::endl;
                            sync_timeout_logged_.store(true, std::memory_order_release);
                        }
                    }
                    else
                    {
                        sync_timeout_logged_.store(false, std::memory_order_release);
                    }
                }
            }

            if (opt_.pdelay_timeout_ms > 0)
            {
                const bool waiting = pdelay_waiting_resp_.load(std::memory_order_acquire);
                const std::int64_t req_ts = last_pdelay_req_mono_ns_.load(std::memory_order_acquire);

                if (waiting && req_ts > 0)
                {
                    const std::int64_t delta = mono_now - req_ts;
                    const std::int64_t timeout_ns = static_cast<std::int64_t>(opt_.pdelay_timeout_ms) * 1'000'000LL;

                    if (delta > timeout_ns)
                    {
                        if (!pdelay_timeout_logged_.load(std::memory_order_acquire))
                        {
                            const auto cnt = pdelay_consecutive_loss_count_.load(std::memory_order_acquire) + 1;
                            pdelay_consecutive_loss_count_.store(cnt, std::memory_order_release);
                            std::cout << "[WARN] Pdelay response timeout, consecutive lost count="
                                      << cnt << std::endl;
                            pdelay_timeout_logged_.store(true, std::memory_order_release);
                        }
                        pdelay_waiting_resp_.store(false, std::memory_order_release);
                    }
                    else
                    {
                        pdelay_timeout_logged_.store(false, std::memory_order_release);
                    }
                }
                else
                {
                    pdelay_timeout_logged_.store(false, std::memory_order_release);
                }
            }

            if (abs_sock_fd_ >= 0)
            {
                for (;;)
                {
                    ExternalAbsMsg msg{};
                    const ssize_t r = ::recv(abs_sock_fd_, &msg, sizeof(msg), 0);
                    if (r < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        break;
                    }
                    if (static_cast<std::size_t>(r) < sizeof(ExternalAbsMsg))
                        continue;
                    if (msg.magic != kAbsMsgMagic || msg.version != kAbsMsgVer)
                        continue;

                    abs_ext_.utc_ns = msg.utc_ns;
                    abs_ext_.inaccuracy_ns = msg.inaccuracy_ns;
                    abs_ext_.sec = static_cast<score_time::AbsoluteSecurityQualifier>(msg.sec_qual);
                    abs_ext_.mono_rx_ns = score_time::utils::ClockNs(CLOCK_MONOTONIC);
                    abs_ext_.valid = true;
                }
            }

            if (!opt_.ntp_servers.empty() &&
                (next_ntp_query_mono_ns_ == 0 || mono_now >= next_ntp_query_mono_ns_))
            {
                auto s = ntp_client_.QueryOnce();
                if (s)
                {
                    ntp_estimator_.Update(*s);
                    if (shared_)
                    {
                        const auto est = ntp_estimator_.Snapshot();
                        score_time::ipc::LogAbsolute(*shared_, mono_now, score_time::SyncLogEvent::kAbsUpdate,
                                                     est.inaccuracy_ns, /*v2=*/2);
                    }
                }
                else
                {
                    ntp_estimator_.MarkTimeout(mono_now);
                }
                next_ntp_query_mono_ns_ = mono_now + static_cast<std::int64_t>(opt_.ntp_query_interval_ms) * 1'000'000LL;
            }

            const bool ext_ok =
                (abs_sock_fd_ >= 0) && abs_ext_.valid &&
                (mono_now - abs_ext_.mono_rx_ns) <= (static_cast<std::int64_t>(opt_.abs_source_timeout_ms) * 1'000'000LL);

            if (ext_ok)
            {
                PublishAbsoluteFromExternal();
            }
            else
            {
                PublishAbsoluteFromNtp();
            }

            if (shared_)
            {
                const auto acc = shared_->abs_acc.load(std::memory_order_acquire);
                const auto sec = shared_->abs_sec.load(std::memory_order_acquire);
                const auto src = shared_->abs_source.load(std::memory_order_acquire);
                const std::int64_t h = (static_cast<std::int64_t>(acc) << 32) |
                                       (static_cast<std::int64_t>(sec) << 16) |
                                       static_cast<std::int64_t>(src);
                if (h != last_state_hash)
                {
                    score_time::ipc::LogAbsolute(*shared_, mono_now, score_time::SyncLogEvent::kAbsState,
                                                 static_cast<std::int64_t>(acc), static_cast<std::int64_t>(sec));
                    last_state_hash = h;
                }
            }

            // Calculate next wake time
            const std::int64_t next_ns = timespec_to_tmv(next).ns + interval_ns;
            next.tv_sec = static_cast<time_t>(next_ns / 1'000'000'000LL);
            next.tv_nsec = static_cast<long>(next_ns % 1'000'000'000LL);

            ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
        }
    }

    score_time::AbsoluteAccuracyQualifier TimeSyncEngine::MapInaccuracyToQual(std::int64_t inacc_ns) const
    {
        if (inacc_ns <= 0)
            return score_time::AbsoluteAccuracyQualifier::kInaccuracyNotAvailable;

        const std::int64_t s = 1'000'000'000LL;
        const std::int64_t min = 60LL * s;
        const std::int64_t h = 60LL * min;
        const std::int64_t d = 24LL * h;

        if (inacc_ns > d)
            return score_time::AbsoluteAccuracyQualifier::kInaccGreaterThan24h;
        if (inacc_ns > h)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan24h;
        if (inacc_ns > 15LL * min)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan1h;
        if (inacc_ns > 60LL * s)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan15min;
        if (inacc_ns > 10LL * s)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan60s;
        if (inacc_ns > 1LL * s)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan10s;
        if (inacc_ns > 500'000'000LL)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan1s;
        if (inacc_ns > 100'000'000LL)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan500ms;
        if (inacc_ns > 50'000'000LL)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan100ms;
        if (inacc_ns > 10'000'000LL)
            return score_time::AbsoluteAccuracyQualifier::kInaccLessThan50ms;
        return score_time::AbsoluteAccuracyQualifier::kInaccLessThan10ms;
    }

    void TimeSyncEngine::PublishAbsoluteFromExternal()
    {
        if (!shared_)
            return;
        const std::int64_t mono_now = score_time::utils::ClockNs(CLOCK_MONOTONIC);
        const std::int64_t utc = abs_ext_.utc_ns;
        const std::int64_t inacc = abs_ext_.inaccuracy_ns;
        const auto acc = MapInaccuracyToQual(inacc);
        const auto sec = abs_ext_.sec;
        score_time::ipc::WriteAbsolute(*shared_, utc, mono_now, acc, sec, inacc,
                                       /*offset_ns_est=*/0,
                                       /*jitter_ns_est=*/0,
                                       /*last_update_mono_ns=*/abs_ext_.mono_rx_ns,
                                       /*source=*/1);
        score_time::ipc::LogAbsolute(*shared_, mono_now, score_time::SyncLogEvent::kAbsUpdate, inacc, 1);
    }

    void TimeSyncEngine::PublishAbsoluteFromNtp()
    {
        if (!shared_)
            return;

        const std::int64_t mono_now = score_time::utils::ClockNs(CLOCK_MONOTONIC);
        const auto est = ntp_estimator_.Snapshot();

        if (!est.locked)
        {
            score_time::ipc::WriteAbsolute(*shared_, 0, 0,
                                           score_time::AbsoluteAccuracyQualifier::kInaccuracyNotAvailable,
                                           score_time::AbsoluteSecurityQualifier::kNoTimeAvailable,
                                           0, 0, 0, est.last_update_mono_ns,
                                           /*source=*/2);
            return;
        }

        const std::int64_t rt_now = score_time::utils::ClockNs(CLOCK_REALTIME);
        const std::int64_t utc_est = rt_now + est.offset_ns;

        const auto acc = MapInaccuracyToQual(est.inaccuracy_ns);
        const auto sec = score_time::AbsoluteSecurityQualifier::kNotTrustworthy;

        score_time::ipc::WriteAbsolute(*shared_, utc_est, mono_now, acc, sec,
                                       est.inaccuracy_ns, est.offset_ns, est.jitter_ns, est.last_update_mono_ns,
                                       /*source=*/2);

        score_time::ipc::LogAbsolute(*shared_, mono_now, score_time::SyncLogEvent::kAbsOffset,
                                     est.offset_ns, est.inaccuracy_ns);
    }

    void TimeSyncEngine::HandlePacket(const unsigned char *frame, int frame_len, const ::timespec &hwts)
    {
        int eth_offset = 0;
        PTPMessage msg{};
        ::ethhdr eth{};
        parse_ethernet_header(frame, eth, eth_offset);
        if (frame_len <= eth_offset)
            return;

        if (!parse_gptp_message(frame + eth_offset, static_cast<std::size_t>(frame_len - eth_offset), msg))
            return;

        {
            const std::uint8_t tsmt = msg.ptpHdr.tsmt;
            const std::uint8_t transportSpecific = static_cast<std::uint8_t>(tsmt & 0xF0U);
            if (transportSpecific != PTP_TRANSPORT_SPECIFIC)
            {
                std::cout << "[WARN] Invalid transportSpecific field: "
                          << static_cast<int>(transportSpecific)
                          << " (expected " << static_cast<int>(PTP_TRANSPORT_SPECIFIC) << ")"
                          << std::endl;
            }
        }

        switch (msg.msgtype)
        {
        case PTP_MSGTYPE_SYNC:
            msg.recvHardwareTS = timespec_to_tmv(hwts);
            SyncFupStateMachine(TsyncEvent::kRecvSync, msg);
            break;
        case PTP_MSGTYPE_FOLLOW_UP:
            msg.parseMessageTs = Timestamp_to_tmv(msg.follow_up.preciseOriginTimestamp);
            SyncFupStateMachine(TsyncEvent::kRecvFup, msg);
            break;
        case PTP_MSGTYPE_PDELAY_REQ:
            {
                score_time::utils::PthreadLockGuard lock(&ctx_.pdelay_lock);
                (void)SendPdelayRespAndFup(msg);
            }
            break;
        case PTP_MSGTYPE_PDELAY_RESP:
            msg.recvHardwareTS = timespec_to_tmv(hwts);
            msg.parseMessageTs = Timestamp_to_tmv(msg.pdelay_resp.responseOriginTimestamp);
            ctx_.peer_delay_resp = msg;
            break;
        case PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP:
            msg.parseMessageTs = Timestamp_to_tmv(msg.pdelay_resp_fup.responseOriginReceiptTimestamp);
            ctx_.peer_delay_fup = msg;
            {
                score_time::utils::PthreadLockGuard lock(&ctx_.pdelay_lock);
                ComputePeerDelay();
            }
            break;
        default:
            break;
        }
    }

    void TimeSyncEngine::SyncFupStateMachine(TsyncEvent ev, const PTPMessage &msg)
    {
        switch (ctx_.state)
        {
        case TsyncState::kEmpty:
            if (ev == TsyncEvent::kRecvSync)
            {
                ctx_.last_sync = msg;
                ctx_.state = TsyncState::kHaveSync;
            }
            else if (ev == TsyncEvent::kRecvFup)
            {
                ctx_.last_fup = msg;
                ctx_.state = TsyncState::kHaveFup;
            }
            break;
        case TsyncState::kHaveSync:
            if (ev == TsyncEvent::kRecvSync)
            {
                ctx_.last_sync = msg;
            }
            else if (ev == TsyncEvent::kRecvFup)
            {
                const auto &sync = ctx_.last_sync;
                if (sync.ptpHdr.sequenceId == msg.ptpHdr.sequenceId)
                {
                    PortSynchronize(sync.recvHardwareTS,
                                    msg.parseMessageTs,
                                    correction_to_tmv(sync.ptpHdr.correctionField),
                                    correction_to_tmv(msg.ptpHdr.correctionField),
                                    msg.ptpHdr.sequenceId);
                    ctx_.state = TsyncState::kEmpty;
                }
                else
                {
                    ctx_.last_fup = msg;
                    ctx_.state = TsyncState::kHaveFup;
                }
            }
            break;
        case TsyncState::kHaveFup:
            if (ev == TsyncEvent::kRecvFup)
            {
                ctx_.last_fup = msg;
            }
            else if (ev == TsyncEvent::kRecvSync)
            {
                ctx_.last_sync = msg;
                ctx_.state = TsyncState::kHaveSync;
            }
            break;
        }
    }

    int TimeSyncEngine::SendPdelayRequest()
    {
        PTPMessage req{};
        ::timespec hwts{};

        req.ptpHdr.tsmt = PTP_MSGTYPE_PDELAY_REQ | PTP_TRANSPORT_SPECIFIC;
        req.ptpHdr.version = PTP_VERSION;
        req.ptpHdr.domainNumber = 0;
        req.ptpHdr.messageLength = htons(sizeof(PdelayReqBody));
        req.ptpHdr.reserved1 = 0;
        req.ptpHdr.flagField[0] = 0;
        req.ptpHdr.flagField[1] = 0;
        req.ptpHdr.correctionField = htobe64(0);
        req.ptpHdr.reserved2 = 0;
        req.ptpHdr.sourcePortIdentity.clockIdentity = ctx_.clockIdentity;
        req.ptpHdr.sourcePortIdentity.portNumber = htons(0x01);
        req.ptpHdr.sequenceId = htons(static_cast<std::uint16_t>(ctx_.pdelay_seqnum++));
        req.ptpHdr.controlField = CTL_OTHER;
        req.ptpHdr.logMessageInterval = 0x7F;

        ctx_.peer_delay_req = req;
        ctx_.peer_delay_req.ptpHdr.sequenceId = ntohs(ctx_.peer_delay_req.ptpHdr.sequenceId);

        unsigned int len = sizeof(PdelayReqBody);
        add_ethernet_header(reinterpret_cast<unsigned char *>(&req), len);
        const int r = raw_sendMsg(ctx_.raw_fd, &req, static_cast<int>(len), &hwts);
        ctx_.peer_delay_req.sendHardwareTS = timespec_to_tmv(hwts);

        const std::int64_t mono_now = score_time::utils::ClockNs(CLOCK_MONOTONIC);
        last_pdelay_req_mono_ns_.store(mono_now, std::memory_order_release);
        pdelay_waiting_resp_.store(true, std::memory_order_release);

        (void)r;
        return r;
    }

    int TimeSyncEngine::SendPdelayRespAndFup(const PTPMessage &req_in)
    {
        PTPMessage rsp{};
        PTPMessage fup{};
        ::timespec hwts{};

        rsp.ptpHdr.tsmt = PTP_MSGTYPE_PDELAY_RESP | PTP_TRANSPORT_SPECIFIC;
        rsp.ptpHdr.version = PTP_VERSION;
        rsp.ptpHdr.domainNumber = req_in.ptpHdr.domainNumber;
        rsp.ptpHdr.messageLength = htons(sizeof(PdelayRespBody));
        rsp.ptpHdr.reserved1 = 0;
        rsp.ptpHdr.flagField[0] = 0x02;
        rsp.ptpHdr.flagField[1] = 0x00;
        rsp.ptpHdr.correctionField = req_in.ptpHdr.correctionField;
        rsp.ptpHdr.reserved2 = 0;
        rsp.ptpHdr.sourcePortIdentity.clockIdentity = ctx_.clockIdentity;
        rsp.ptpHdr.sourcePortIdentity.portNumber = htons(0x01);
        rsp.ptpHdr.sequenceId = htons(req_in.ptpHdr.sequenceId);
        rsp.ptpHdr.controlField = CTL_OTHER;
        rsp.ptpHdr.logMessageInterval = 0x7F;

        rsp.pdelay_resp.responseOriginTimestamp = tmv_to_Timestamp(req_in.recvHardwareTS);
        rsp.pdelay_resp.requestingPortIdentity.clockIdentity = req_in.ptpHdr.sourcePortIdentity.clockIdentity;
        rsp.pdelay_resp.requestingPortIdentity.portNumber = htons(req_in.ptpHdr.sourcePortIdentity.portNumber);

        rsp.pdelay_resp.responseOriginTimestamp.seconds_lsb = htonl(rsp.pdelay_resp.responseOriginTimestamp.seconds_lsb);
        rsp.pdelay_resp.responseOriginTimestamp.seconds_msb = htons(rsp.pdelay_resp.responseOriginTimestamp.seconds_msb);
        rsp.pdelay_resp.responseOriginTimestamp.nanoseconds = htonl(rsp.pdelay_resp.responseOriginTimestamp.nanoseconds);

        unsigned int len = sizeof(PdelayRespBody);
        add_ethernet_header(reinterpret_cast<unsigned char *>(&rsp), len);
        const int r1 = raw_sendMsg(ctx_.raw_fd, &rsp, static_cast<int>(len), &hwts);
        const tmv_t rsp_ts = timespec_to_tmv(hwts);

        fup.ptpHdr.tsmt = PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP | PTP_TRANSPORT_SPECIFIC;
        fup.ptpHdr.version = PTP_VERSION;
        fup.ptpHdr.domainNumber = req_in.ptpHdr.domainNumber;
        fup.ptpHdr.messageLength = htons(sizeof(PdelayRespFollowUpBody));
        fup.ptpHdr.reserved1 = 0;
        fup.ptpHdr.flagField[0] = 0;
        fup.ptpHdr.flagField[1] = 0;
        fup.ptpHdr.correctionField = req_in.ptpHdr.correctionField;
        fup.ptpHdr.reserved2 = 0;
        fup.ptpHdr.sourcePortIdentity.clockIdentity = ctx_.clockIdentity;
        fup.ptpHdr.sourcePortIdentity.portNumber = htons(0x01);
        fup.ptpHdr.sequenceId = htons(req_in.ptpHdr.sequenceId);
        fup.ptpHdr.controlField = CTL_OTHER;
        fup.ptpHdr.logMessageInterval = 0x7F;

        fup.pdelay_resp_fup.responseOriginReceiptTimestamp = tmv_to_Timestamp(rsp_ts);
        fup.pdelay_resp_fup.requestingPortIdentity.clockIdentity = req_in.ptpHdr.sourcePortIdentity.clockIdentity;
        fup.pdelay_resp_fup.requestingPortIdentity.portNumber = htons(req_in.ptpHdr.sourcePortIdentity.portNumber);

        fup.pdelay_resp_fup.responseOriginReceiptTimestamp.seconds_lsb = htonl(fup.pdelay_resp_fup.responseOriginReceiptTimestamp.seconds_lsb);
        fup.pdelay_resp_fup.responseOriginReceiptTimestamp.seconds_msb = htons(fup.pdelay_resp_fup.responseOriginReceiptTimestamp.seconds_msb);
        fup.pdelay_resp_fup.responseOriginReceiptTimestamp.nanoseconds = htonl(fup.pdelay_resp_fup.responseOriginReceiptTimestamp.nanoseconds);

        len = sizeof(PdelayRespFollowUpBody);
        add_ethernet_header(reinterpret_cast<unsigned char *>(&fup), len);
        const int r2 = raw_sendMsg(ctx_.raw_fd, &fup, static_cast<int>(len), &hwts);
        (void)r1;
        return r2;
    }

    void TimeSyncEngine::ComputePeerDelay()
    {
        auto &req = ctx_.peer_delay_req;
        auto &rsp = ctx_.peer_delay_resp;
        auto &fup = ctx_.peer_delay_fup;

        if (req.ptpHdr.sequenceId != rsp.ptpHdr.sequenceId)
            return;
        if (rsp.ptpHdr.sequenceId != fup.ptpHdr.sequenceId)
            return;

        const tmv_t t1 = req.sendHardwareTS;
        const tmv_t t2 = rsp.parseMessageTs;
        const tmv_t t3 = fup.parseMessageTs;
        const tmv_t t4 = rsp.recvHardwareTS;

        const tmv_t c1 = correction_to_tmv(rsp.ptpHdr.correctionField);
        const tmv_t c2 = correction_to_tmv(fup.ptpHdr.correctionField);
        const tmv_t t3c{t3.ns + c1.ns + c2.ns};

        ctx_.path_delay = ((t2.ns - t1.ns) + (t4.ns - t3c.ns)) / 2;
        std::cout << "[DEBUG] ComputePeerDelay: t1=" << t1.ns
                  << " t2=" << t2.ns
                  << " t3=" << t3.ns
                  << " t4=" << t4.ns
                  << " path_delay=" << ctx_.path_delay << std::endl;

        const std::int64_t mono_now = score_time::utils::ClockNs(CLOCK_MONOTONIC);
        last_pdelay_event_mono_ns_.store(mono_now, std::memory_order_release);
        pdelay_waiting_resp_.store(false, std::memory_order_release);
        pdelay_consecutive_loss_count_.store(0, std::memory_order_release);

        if (shared_)
        {
            shared_->vehicle_path_delay_ns.store(ctx_.path_delay, std::memory_order_release);
            score_time::ipc::LogVehicle(*shared_, mono_now, score_time::SyncLogEvent::kVehiclePeerDelay,
                                        ctx_.path_delay, 0);
        }
    }

    void TimeSyncEngine::PortSynchronize(const tmv_t &sync_hw_ts,
                                         const tmv_t &fup_msg_ts,
                                         const tmv_t &sync_corr,
                                         const tmv_t &fup_corr,
                                         std::uint16_t seq_id)
    {
        const std::int64_t master_ns = fup_msg_ts.ns + sync_corr.ns + fup_corr.ns;
        if (master_ns <= 0)
            return;

        score_time::AccuracyQualifier acc = score_time::AccuracyQualifier::kSynchronized;

        const std::int64_t delta = master_ns - ctx_.last_master_ts;
        if (ctx_.last_master_ts != 0 && delta <= 0)
        {
            acc = score_time::AccuracyQualifier::kTimeJumpDetected;
        }
        else if (ctx_.last_master_ts != 0 && delta > opt_.jump_future_threshold_ns)
        {
            acc = score_time::AccuracyQualifier::kUnstable;
        }

        const std::int64_t offset = sync_hw_ts.ns - master_ns;

        std::cout << "[DEBUG] PortSynchronize: sync_hw_ts=" << sync_hw_ts.ns
                  << " master_ns=" << master_ns
                  << " offset=" << offset << std::endl;

        if (std::llabs(offset) > opt_.unstable_offset_threshold_ns &&
            acc == score_time::AccuracyQualifier::kSynchronized)
        {
            acc = score_time::AccuracyQualifier::kUnstable;
        }

#ifndef _QNX_PLAT
        if (ctx_.phc_fd >= 0)
        {
            ::timespec ts{};
            if (::clock_gettime(ctx_.clk_id, &ts) == 0)
            {
                ts.tv_nsec -= offset;
                normalize_timespec(ts);
                if (::clock_settime(ctx_.clk_id, &ts) != 0)
                {
                    std::fprintf(stderr, "tsyncd: clock_settime failed: %s\n", std::strerror(errno));
                    acc = score_time::AccuracyQualifier::kNotSynchronized;
                }
            }
        }
#else
        (void)qnx_phc_adjtime_step(ctx_.phc_fd, -offset);
#endif

        ctx_.last_master_ts = master_ns;

        const score_time::TimePointQualifier tpq =
            (acc == score_time::AccuracyQualifier::kSynchronized)
                ? score_time::TimePointQualifier::kASIL_B
                : score_time::TimePointQualifier::kQM;

        const std::int64_t mono_ns = score_time::utils::ClockNs(CLOCK_MONOTONIC);

        last_sync_event_mono_ns_.store(mono_ns, std::memory_order_release);

        if (shared_)
        {
            const auto prev_base = shared_->vehicle_base_ns.load(std::memory_order_acquire);
            score_time::AccuracyQualifier acc2 = acc;
            if (prev_base != 0 && master_ns < prev_base)
            {
                acc2 = score_time::AccuracyQualifier::kTimeJumpDetected;
            }

            score_time::ipc::WriteVehicle(*shared_, master_ns, mono_ns, acc2, tpq, offset, ctx_.path_delay);
            score_time::ipc::LogVehicle(*shared_, mono_ns, score_time::SyncLogEvent::kVehicleOffset, offset, ctx_.path_delay);
            score_time::ipc::LogVehicle(*shared_, mono_ns, score_time::SyncLogEvent::kVehicleState,
                                        static_cast<std::int64_t>(acc2), static_cast<std::int64_t>(tpq));
        }

        (void)seq_id;
    }

    tmv_t TimeSyncEngine::timespec_to_tmv(const ::timespec &ts)
    {
        return tmv_t{static_cast<std::int64_t>(ts.tv_sec) * NS_PER_SEC + ts.tv_nsec};
    }

    tmv_t TimeSyncEngine::correction_to_tmv(std::int64_t corr)
    {
        return tmv_t{corr >> 16};
    }

    tmv_t TimeSyncEngine::Timestamp_to_tmv(const Timestamp &ts)
    {
        const std::uint64_t sec = (static_cast<std::uint64_t>(ts.seconds_msb) << 32) |
                                  static_cast<std::uint64_t>(ts.seconds_lsb);
        return tmv_t{static_cast<std::int64_t>(sec * static_cast<std::uint64_t>(NS_PER_SEC) + ts.nanoseconds)};
    }

    Timestamp TimeSyncEngine::tmv_to_Timestamp(const tmv_t &x)
    {
        Timestamp t{};
        const std::uint64_t sec = static_cast<std::uint64_t>(x.ns) / 1'000'000'000ULL;
        const std::uint64_t nsec = static_cast<std::uint64_t>(x.ns) % 1'000'000'000ULL;
        t.seconds_lsb = static_cast<std::uint32_t>(sec & 0xFFFFFFFFULL);
        t.seconds_msb = static_cast<std::uint16_t>((sec >> 32) & 0xFFFFULL);
        t.nanoseconds = static_cast<std::uint32_t>(nsec);
        return t;
    }

    void TimeSyncEngine::normalize_timespec(::timespec &ts)
    {
        while (ts.tv_nsec >= 1'000'000'000L)
        {
            ts.tv_nsec -= 1'000'000'000L;
            ts.tv_sec += 1;
        }
        while (ts.tv_nsec < 0)
        {
            ts.tv_nsec += 1'000'000'000L;
            ts.tv_sec -= 1;
        }
    }

}
