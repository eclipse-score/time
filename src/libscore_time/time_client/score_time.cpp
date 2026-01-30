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
#include "score_time/score_time.hpp"

#include "score_time/ipc/shm_region.hpp"

#include <time.h>
#include <atomic>
#include <ctime>
#include <cstring>

namespace score_time
{

    inline std::int64_t ReadClockNs(clockid_t clk)
    {
        ::timespec ts{};
        ::clock_gettime(clk, &ts);
        return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec;
    }

    class MonotonicClockImpl final : public IMonotonicClock
    {
    public:
        TimePoint<MonotonicClockTag> Now() const override
        {
            return TimePoint<MonotonicClockTag>(ReadClockNs(CLOCK_MONOTONIC));
        }
    };

    class HighPrecisionClockImpl final : public IHighPrecisionClock
    {
    public:
        explicit HighPrecisionClockImpl(Options::HighPrecisionMapping m) : mapping_(m) {}
        TimePoint<HighPrecisionClockTag> Now() const override
        {
#ifdef CLOCK_TAI
            const clockid_t clk = (mapping_ == Options::HighPrecisionMapping::kTAI) ? CLOCK_TAI : CLOCK_REALTIME;
#else
            const clockid_t clk = CLOCK_REALTIME;
#endif
            return TimePoint<HighPrecisionClockTag>(ReadClockNs(clk));
        }

    private:
        Options::HighPrecisionMapping mapping_;
    };

    class VehicleTimeImpl final : public IVehicleTime
    {
    public:
        explicit VehicleTimeImpl(const ipc::SharedState *s) : s_(s) {}

        TimePoint<VehicleClockTag> Now() const override
        {
            if (!s_)
                return TimePoint<VehicleClockTag>(0);

            std::int64_t base_vehicle{}, base_mono{};
            AccuracyQualifier acc{};
            TimePointQualifier tpq{};
            if (!ipc::ReadVehicle(*s_, base_vehicle, base_mono, acc, tpq))
            {
                return TimePoint<VehicleClockTag>(0);
            }
            (void)tpq;
            if (acc == AccuracyQualifier::kNoTime || acc == AccuracyQualifier::kNotSynchronized)
            {
                return TimePoint<VehicleClockTag>(0);
            }
            const auto mono_now = ReadClockNs(CLOCK_MONOTONIC);
            return TimePoint<VehicleClockTag>(base_vehicle + (mono_now - base_mono));
        }

        AccuracyQualifier GetAccuracyQualifier() const override
        {
            return s_ ? s_->vehicle_acc.load(std::memory_order_acquire) : AccuracyQualifier::kNoTime;
        }

        TimePointQualifier GetTimePointQualifier() const override
        {
            return s_ ? s_->vehicle_tpq.load(std::memory_order_acquire) : TimePointQualifier::kQM;
        }

    private:
        const ipc::SharedState *s_;
    };

    class AbsoluteTimeImpl final : public IAbsoluteTime
    {
    public:
        explicit AbsoluteTimeImpl(const ipc::SharedState *s) : s_(s) {}

        TimePoint<AbsoluteClockTag> Now() const override
        {
            if (!s_)
                return TimePoint<AbsoluteClockTag>(0);
            std::int64_t base_utc{}, base_mono{}, inacc{};
            AbsoluteAccuracyQualifier acc{};
            AbsoluteSecurityQualifier sec{};
            std::uint8_t source{};
            if (!ipc::ReadAbsolute(*s_, base_utc, base_mono, acc, sec, inacc, source))
            {
                return TimePoint<AbsoluteClockTag>(0);
            }
            (void)acc;
            (void)source;
            if (sec == AbsoluteSecurityQualifier::kNoTimeAvailable)
            {
                return TimePoint<AbsoluteClockTag>(0);
            }
            const auto mono_now = ReadClockNs(CLOCK_MONOTONIC);
            return TimePoint<AbsoluteClockTag>(base_utc + (mono_now - base_mono));
        }

        AbsoluteAccuracyQualifier GetAccuracyQualifier() const override
        {
            return s_ ? s_->abs_acc.load(std::memory_order_acquire)
                      : AbsoluteAccuracyQualifier::kInaccuracyNotAvailable;
        }

        AbsoluteSecurityQualifier GetSecurityQualifier() const override
        {
            return s_ ? s_->abs_sec.load(std::memory_order_acquire)
                      : AbsoluteSecurityQualifier::kNoTimeAvailable;
        }

        std::int64_t GetEstimatedInaccuracyNs() const override
        {
            return s_ ? s_->abs_inaccuracy_ns.load(std::memory_order_acquire) : 0;
        }

    private:
        const ipc::SharedState *s_;
    };

    static std::size_t ReadLogRing(const ipc::SyncLogEntry *ring,
                                   std::size_t capacity,
                                   std::uint32_t head,
                                   SyncLogEntry *out,
                                   std::size_t out_cap)
    {
        if (!out || out_cap == 0)
            return 0;

        const std::size_t available = (head < capacity) ? head : capacity;
        const std::size_t n = (available < out_cap) ? available : out_cap;
        // Newest -> oldest
        for (std::size_t i = 0; i < n; ++i)
        {
            const std::uint32_t idx = head - 1u - static_cast<std::uint32_t>(i);
            const std::size_t r = static_cast<std::size_t>(idx % capacity);
            const auto &e = ring[r];
            out[i] = SyncLogEntry{e.monotonic_ns, static_cast<SyncLogEvent>(e.type), e.v1, e.v2};
        }
        return n;
    }

    class ScoreTimeImpl final : public ScoreTime
    {
    public:
        explicit ScoreTimeImpl(const Options &opt)
            : opt_(opt),
              high_prec_(opt.high_precision_mapping) {}

        bool Init()
        {
            if (!shm_.Open(opt_.shm_name, opt_.shm_size, /*create_or_open=*/false))
            {
                state_ = nullptr;
                vehicle_ = VehicleTimeImpl(nullptr);
                absolute_ = AbsoluteTimeImpl(nullptr);
                return true;
            }
            state_ = reinterpret_cast<ipc::SharedState *>(shm_.Addr());

            // Basic validation
            const auto magic = state_->magic.load(std::memory_order_acquire);
            const auto ver = state_->version.load(std::memory_order_acquire);
            if (magic != ipc::SharedState::kMagic)
            {
                state_ = nullptr;
            }
            else if (ver != ipc::SharedState::kVersion && ver != 1)
            {
                // Allow reading legacy v1 (vehicle only)
                state_ = nullptr;
            }

            vehicle_ = VehicleTimeImpl(state_);
            absolute_ = AbsoluteTimeImpl((ver == ipc::SharedState::kVersion) ? state_ : nullptr);
            return true;
        }

        const IVehicleTime &VehicleTime() const override { return vehicle_; }
        const IAbsoluteTime &AbsoluteTime() const override { return absolute_; }
        const IHighPrecisionClock &HighPrecisionClock() const override { return high_prec_; }
        const IMonotonicClock &MonotonicClock() const override { return mono_; }

        std::int64_t LastOffsetNs() const override
        {
            return state_ ? state_->vehicle_last_offset_ns.load(std::memory_order_acquire) : 0;
        }
        std::int64_t PathDelayNs() const override
        {
            return state_ ? state_->vehicle_path_delay_ns.load(std::memory_order_acquire) : 0;
        }

        std::size_t ReadVehicleSyncLog(SyncLogEntry *out, std::size_t capacity) const override
        {
            if (!state_)
                return 0;
            const auto head = state_->vehicle_log_head.load(std::memory_order_acquire);
            return ReadLogRing(state_->vehicle_log, ipc::kVehicleLogCapacity, head, out, capacity);
        }
        std::size_t ReadAbsoluteSyncLog(SyncLogEntry *out, std::size_t capacity) const override
        {
            if (!state_)
                return 0;
            const auto ver = state_->version.load(std::memory_order_acquire);
            if (ver != ipc::SharedState::kVersion)
                return 0;
            const auto head = state_->abs_log_head.load(std::memory_order_acquire);
            return ReadLogRing(state_->abs_log, ipc::kAbsLogCapacity, head, out, capacity);
        }

    private:
        Options opt_;
        mutable ipc::ShmRegion shm_;
        ipc::SharedState *state_ = nullptr;

        MonotonicClockImpl mono_{};
        HighPrecisionClockImpl high_prec_;
        VehicleTimeImpl vehicle_{nullptr};
        AbsoluteTimeImpl absolute_{nullptr};
    };

    std::unique_ptr<ScoreTime> ScoreTime::Create(const Options &opt)
    {
        auto impl = std::make_unique<ScoreTimeImpl>(opt);
        if (!impl->Init())
            return nullptr;
        return impl;
    }

}
