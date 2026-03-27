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
#include "score/TimeSlave/code/gptp/gptp_engine.h"
#include "score/TimeSlave/code/gptp/details/network_identity.h"
#include "score/TimeSlave/code/gptp/details/raw_socket.h"

#include "score/TimeDaemon/code/common/logging_contexts.h"
#include "score/mw/log/logging.h"

#include <time.h>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

constexpr int kRxTimeoutMs = 100;  // poll timeout; keeps RxLoop responsive to shutdown
constexpr int kRxBufferSize = 2048;

std::int64_t MonoNs() noexcept
{
    ::timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec;
}

}  // namespace

GptpEngine::GptpEngine(GptpEngineOptions opts,
                       std::unique_ptr<score::td::PtpTimeInfo::ReferenceClock> local_clock) noexcept
    : opts_{std::move(opts)},
      local_clock_{std::move(local_clock)},
      socket_{std::make_unique<RawSocket>()},
      identity_{std::make_unique<NetworkIdentity>()},
      codec_{},
      parser_{},
      sync_sm_{opts_.jump_future_threshold_ns},
      pdelay_{nullptr}
{
}

GptpEngine::GptpEngine(GptpEngineOptions opts,
                       std::unique_ptr<score::td::PtpTimeInfo::ReferenceClock> local_clock,
                       std::unique_ptr<IRawSocket> socket,
                       std::unique_ptr<INetworkIdentity> identity) noexcept
    : opts_{std::move(opts)},
      local_clock_{std::move(local_clock)},
      socket_{std::move(socket)},
      identity_{std::move(identity)},
      codec_{},
      parser_{},
      sync_sm_{opts_.jump_future_threshold_ns},
      pdelay_{nullptr}
{
}

GptpEngine::~GptpEngine() noexcept
{
    (void)Deinitialize();
}

bool GptpEngine::Initialize()
{
    if (running_.load(std::memory_order_acquire))
        return true;

    if (!identity_->Resolve(opts_.iface_name))
    {
        score::mw::log::LogError(score::td::kGPtpMachineContext)
            << "GptpEngine: failed to resolve ClockIdentity for " << opts_.iface_name;
        return false;
    }

    pdelay_ = std::make_unique<PeerDelayMeasurer>(identity_->GetClockIdentity());

    if (!socket_->Open(opts_.iface_name))
    {
        score::mw::log::LogError(score::td::kGPtpMachineContext)
            << "GptpEngine: failed to open raw socket on " << opts_.iface_name;
        return false;
    }

    if (!socket_->EnableHwTimestamping())
    {
        score::mw::log::LogWarn(score::td::kGPtpMachineContext)
            << "GptpEngine: HW timestamping not available on " << opts_.iface_name << ", falling back to SW timestamps";
    }

    running_.store(true, std::memory_order_release);

    if (::pthread_create(&rx_thread_, nullptr, &RxThreadEntry, this) != 0)
    {
        score::mw::log::LogError(score::td::kGPtpMachineContext) << "GptpEngine: failed to create RxThread";
        running_.store(false, std::memory_order_release);
        socket_->Close();
        return false;
    }
    rx_started_ = true;

    if (::pthread_create(&pdelay_thread_, nullptr, &PdelayThreadEntry, this) != 0)
    {
        score::mw::log::LogError(score::td::kGPtpMachineContext) << "GptpEngine: failed to create PdelayThread";
        (void)Deinitialize();
        return false;
    }
    pdelay_started_ = true;

    score::mw::log::LogInfo(score::td::kGPtpMachineContext) << "GptpEngine initialized on " << opts_.iface_name;
    return true;
}

bool GptpEngine::Deinitialize()
{
    running_.store(false, std::memory_order_release);

    // Close the socket first so that the RxThread's poll() unblocks
    socket_->Close();

    if (rx_started_)
    {
        ::pthread_join(rx_thread_, nullptr);
        rx_started_ = false;
    }
    if (pdelay_started_)
    {
        ::pthread_join(pdelay_thread_, nullptr);
        pdelay_started_ = false;
    }

    score::mw::log::LogInfo(score::td::kGPtpMachineContext) << "GptpEngine deinitialized";
    return true;
}

bool GptpEngine::ReadPTPSnapshot(score::td::PtpTimeInfo& info)
{
    if (!running_.load(std::memory_order_acquire))
        return false;

    const std::int64_t mono_now = MonoNs();
    const std::int64_t timeout_ns = static_cast<std::int64_t>(opts_.sync_timeout_ms) * 1'000'000LL;

    const bool timed_out = sync_sm_.IsTimeout(mono_now, timeout_ns);

    std::lock_guard<std::mutex> lk(snapshot_mutex_);
    snapshot_.local_time = local_clock_->Now();
    if (timed_out)
    {
        snapshot_.status.is_synchronized = false;
        snapshot_.status.is_timeout = true;
        snapshot_.status.is_correct = false;
    }
    info = snapshot_;
    return true;
}

void* GptpEngine::RxThreadEntry(void* arg) noexcept
{
    if (arg != nullptr)
        static_cast<GptpEngine*>(arg)->RxLoop();
    return nullptr;
}

void* GptpEngine::PdelayThreadEntry(void* arg) noexcept
{
    if (arg != nullptr)
        static_cast<GptpEngine*>(arg)->PdelayLoop();
    return nullptr;
}

void GptpEngine::RxLoop() noexcept
{
    std::uint8_t buf[kRxBufferSize];
    ::timespec hwts{};

    while (running_.load(std::memory_order_acquire))
    {
        std::memset(&hwts, 0, sizeof(hwts));
        const int n = socket_->Recv(buf, sizeof(buf), hwts, kRxTimeoutMs);
        if (n <= 0)
            continue;
        HandlePacket(buf, n, hwts);
    }
}

void GptpEngine::PdelayLoop() noexcept
{
    ::timespec next{};
    ::clock_gettime(CLOCK_MONOTONIC, &next);
    // Configurable warm-up before first Pdelay_Req (default 2 s)
    const std::int64_t warmup_ns = static_cast<std::int64_t>(opts_.pdelay_warmup_ms) * 1'000'000LL;
    const std::int64_t next_warmup_ns =
        static_cast<std::int64_t>(next.tv_sec) * 1'000'000'000LL + next.tv_nsec + warmup_ns;
    next.tv_sec = static_cast<time_t>(next_warmup_ns / 1'000'000'000LL);
    next.tv_nsec = static_cast<long>(next_warmup_ns % 1'000'000'000LL);

    const std::int64_t interval_ns =
        static_cast<std::int64_t>(opts_.pdelay_interval_ms > 0 ? opts_.pdelay_interval_ms : 1000) * 1'000'000LL;

    while (running_.load(std::memory_order_acquire))
    {
        ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
        if (!running_.load(std::memory_order_acquire))
            break;

        if (pdelay_)
        {
            (void)pdelay_->SendRequest(*socket_);
        }

        const std::int64_t next_ns =
            static_cast<std::int64_t>(next.tv_sec) * 1'000'000'000LL + next.tv_nsec + interval_ns;
        next.tv_sec = static_cast<time_t>(next_ns / 1'000'000'000LL);
        next.tv_nsec = static_cast<long>(next_ns % 1'000'000'000LL);
    }
}

void GptpEngine::HandlePacket(const std::uint8_t* frame, int len, const ::timespec& hwts) noexcept
{
    int ptp_offset = 0;
    if (!codec_.ParseEthernetHeader(frame, len, ptp_offset))
        return;

    const auto* payload = frame + ptp_offset;
    const std::size_t payload_len = static_cast<std::size_t>(len - ptp_offset);

    PTPMessage msg{};
    if (!parser_.Parse(payload, payload_len, msg))
        return;

    const TmvT hw_ts{static_cast<std::int64_t>(hwts.tv_sec) * 1'000'000'000LL + hwts.tv_nsec};

    switch (msg.msgtype)
    {
        case kPtpMsgtypeSync:
            msg.recvHardwareTS = hw_ts;
            sync_sm_.OnSync(msg);
            break;

        case kPtpMsgtypeFollowUp:
            msg.parseMessageTs = TimestampToTmv(msg.follow_up.preciseOriginTimestamp);
            {
                auto result = sync_sm_.OnFollowUp(msg);
                if (result.has_value() && pdelay_)
                {
                    const PDelayResult pdr = pdelay_->GetResult();
                    // IEEE 802.1AS: subtract peer link delay from offset
                    if (pdr.valid)
                    {
                        result->offset_ns -= pdr.path_delay_ns;
                        result->sync_fup_data.pdelay = static_cast<std::uint64_t>(pdr.path_delay_ns);
                    }
                    else
                    {
                        result->sync_fup_data.pdelay = 0U;
                    }
                    UpdateSnapshot(*result, pdr);
                }
            }
            break;

        case kPtpMsgtypePdelayResp:
            msg.recvHardwareTS = hw_ts;
            msg.parseMessageTs = TimestampToTmv(msg.pdelay_resp.responseOriginTimestamp);
            if (pdelay_)
                pdelay_->OnResponse(msg);
            break;

        case kPtpMsgtypePdelayRespFollowUp:
            msg.parseMessageTs = TimestampToTmv(msg.pdelay_resp_fup.responseOriginReceiptTimestamp);
            if (pdelay_)
                pdelay_->OnResponseFollowUp(msg);
            break;

        default:
            break;
    }
}

void GptpEngine::UpdateSnapshot(const SyncResult& sync, const PDelayResult& pdelay) noexcept
{
    std::lock_guard<std::mutex> lk(snapshot_mutex_);

    const std::int64_t local_rx_ns = static_cast<std::int64_t>(sync.sync_fup_data.reference_local_timestamp);
    snapshot_.ptp_assumed_time = std::chrono::nanoseconds{local_rx_ns - sync.offset_ns};
    snapshot_.local_time = local_clock_->Now();
    snapshot_.rate_deviation = sync_sm_.GetNeighborRateRatio();

    snapshot_.status.is_synchronized = true;
    snapshot_.status.is_timeout = false;
    snapshot_.status.is_time_jump_future = sync.is_time_jump_future;
    snapshot_.status.is_time_jump_past = sync.is_time_jump_past;
    snapshot_.status.is_correct = !sync.is_time_jump_future && !sync.is_time_jump_past;

    snapshot_.sync_fup_data = sync.sync_fup_data;
    snapshot_.pdelay_data = pdelay.pdelay_data;
}

}  // namespace details
}  // namespace ts
}  // namespace score
