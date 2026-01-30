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
#include <cstdint>
#include <memory>
#include <string>

#include "score_time/ipc/shared_state.hpp"

namespace score_time
{
    struct VehicleClockTag
    {
    };
    struct AbsoluteClockTag
    {
    };
    struct HighPrecisionClockTag
    {
    };
    struct MonotonicClockTag
    {
    };

    template <typename Tag>
    class TimePoint final
    {
    public:
        constexpr TimePoint() : ns_(0) {}
        explicit constexpr TimePoint(std::int64_t ns_since_epoch) : ns_(ns_since_epoch) {}
        constexpr std::int64_t ns_since_epoch() const { return ns_; }

    private:
        std::int64_t ns_;
    };

    class IVehicleTime
    {
    public:
        virtual ~IVehicleTime() = default;
        virtual TimePoint<VehicleClockTag> Now() const = 0;
        virtual AccuracyQualifier GetAccuracyQualifier() const = 0;
        virtual TimePointQualifier GetTimePointQualifier() const = 0;
    };

    class IAbsoluteTime
    {
    public:
        virtual ~IAbsoluteTime() = default;
        virtual TimePoint<AbsoluteClockTag> Now() const = 0;
        virtual AbsoluteAccuracyQualifier GetAccuracyQualifier() const = 0;
        virtual AbsoluteSecurityQualifier GetSecurityQualifier() const = 0;
        // Best-effort inaccuracy estimate (ns). 0 means unknown.
        virtual std::int64_t GetEstimatedInaccuracyNs() const = 0;
    };

    class IHighPrecisionClock
    {
    public:
        virtual ~IHighPrecisionClock() = default;
        virtual TimePoint<HighPrecisionClockTag> Now() const = 0;
    };

    class IMonotonicClock
    {
    public:
        virtual ~IMonotonicClock() = default;
        virtual TimePoint<MonotonicClockTag> Now() const = 0;
    };

    struct Options
    {
        // IPC shared memory name written by tsyncd
        std::string shm_name = "/score_time_vehicle_time";
        std::size_t shm_size = 16384;

        // Map high precision clock to CLOCK_REALTIME or CLOCK_TAI (if available)
        enum class HighPrecisionMapping : std::uint8_t
        {
            kRealtime = 0,
            kTAI = 1
        };
        HighPrecisionMapping high_precision_mapping = HighPrecisionMapping::kRealtime;
    };

    struct SyncLogEntry
    {
        std::int64_t monotonic_ns{0};
        SyncLogEvent type{SyncLogEvent::kVehicleState};
        std::int64_t v1{0};
        std::int64_t v2{0};
    };

    class ScoreTime
    {
    public:
        static std::unique_ptr<ScoreTime> Create(const Options &opt);

        virtual ~ScoreTime() = default;

        virtual const IVehicleTime &VehicleTime() const = 0;
        virtual const IAbsoluteTime &AbsoluteTime() const = 0;
        virtual const IHighPrecisionClock &HighPrecisionClock() const = 0;
        virtual const IMonotonicClock &MonotonicClock() const = 0;

        virtual std::int64_t LastOffsetNs() const = 0;
        virtual std::int64_t PathDelayNs() const = 0;

        virtual std::size_t ReadVehicleSyncLog(SyncLogEntry *out, std::size_t capacity) const = 0;
        virtual std::size_t ReadAbsoluteSyncLog(SyncLogEntry *out, std::size_t capacity) const = 0;
    };

} // namespace score_time
