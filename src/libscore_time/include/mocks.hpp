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
#pragma once

#include "score_time.hpp"

#include <atomic>

namespace score_time
{
    class ManualMonotonicClock : public IMonotonicClock
    {
    public:
        ManualMonotonicClock() = default;
        explicit ManualMonotonicClock(std::int64_t start_ns) : now_ns_(start_ns) {}

        TimePoint<MonotonicClockTag> Now() const override { return TimePoint<MonotonicClockTag>(now_ns_.load(std::memory_order_acquire)); }
        std::int64_t NowNs() const { return now_ns_.load(std::memory_order_acquire); }
        void SetNowNs(std::int64_t v) { now_ns_.store(v, std::memory_order_release); }
        void AdvanceNs(std::int64_t delta) { now_ns_.fetch_add(delta, std::memory_order_acq_rel); }

    private:
        std::atomic<std::int64_t> now_ns_{0};
    };

    class ManualHighPrecisionClock : public IHighPrecisionClock
    {
    public:
        ManualHighPrecisionClock() = default;
        explicit ManualHighPrecisionClock(std::int64_t start_ns) : now_ns_(start_ns) {}

        TimePoint<HighPrecisionClockTag> Now() const override { return TimePoint<HighPrecisionClockTag>(now_ns_.load(std::memory_order_acquire)); }
        std::int64_t NowNs() const { return now_ns_.load(std::memory_order_acquire); }
        void SetNowNs(std::int64_t v) { now_ns_.store(v, std::memory_order_release); }
        void AdvanceNs(std::int64_t delta) { now_ns_.fetch_add(delta, std::memory_order_acq_rel); }

    private:
        std::atomic<std::int64_t> now_ns_{0};
    };

    class ManualVehicleTime : public IVehicleTime
    {
    public:
        TimePoint<VehicleClockTag> Now() const override { return TimePoint<VehicleClockTag>(now_ns_.load(std::memory_order_acquire)); }
        AccuracyQualifier GetAccuracyQualifier() const override { return acc_.load(std::memory_order_acquire); }
        TimePointQualifier GetTimePointQualifier() const override { return tpq_.load(std::memory_order_acquire); }

        void SetNowNs(std::int64_t v) { now_ns_.store(v, std::memory_order_release); }
        void SetAccuracy(AccuracyQualifier v) { acc_.store(v, std::memory_order_release); }
        void SetTimePointQualifier(TimePointQualifier v) { tpq_.store(v, std::memory_order_release); }

    private:
        std::atomic<std::int64_t> now_ns_{0};
        std::atomic<AccuracyQualifier> acc_{AccuracyQualifier::kNoTime};
        std::atomic<TimePointQualifier> tpq_{TimePointQualifier::kQM};
    };

    class ManualAbsoluteTime : public IAbsoluteTime
    {
    public:
        TimePoint<AbsoluteClockTag> Now() const override { return TimePoint<AbsoluteClockTag>(now_utc_ns_.load(std::memory_order_acquire)); }
        AbsoluteAccuracyQualifier GetAccuracyQualifier() const override { return acc_.load(std::memory_order_acquire); }
        AbsoluteSecurityQualifier GetSecurityQualifier() const override { return sec_.load(std::memory_order_acquire); }
        std::int64_t GetEstimatedInaccuracyNs() const override { return inacc_ns_.load(std::memory_order_acquire); }

        void SetNowUtcNs(std::int64_t v) { now_utc_ns_.store(v, std::memory_order_release); }
        void SetAccuracy(AbsoluteAccuracyQualifier v) { acc_.store(v, std::memory_order_release); }
        void SetSecurity(AbsoluteSecurityQualifier v) { sec_.store(v, std::memory_order_release); }
        void SetEstimatedInaccuracyNs(std::int64_t v) { inacc_ns_.store(v, std::memory_order_release); }

    private:
        std::atomic<std::int64_t> now_utc_ns_{0};
        std::atomic<AbsoluteAccuracyQualifier> acc_{AbsoluteAccuracyQualifier::kInaccuracyNotAvailable};
        std::atomic<AbsoluteSecurityQualifier> sec_{AbsoluteSecurityQualifier::kNoTimeAvailable};
        std::atomic<std::int64_t> inacc_ns_{-1};
    };

}
