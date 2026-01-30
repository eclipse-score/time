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

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace score_time
{

    enum class AccuracyQualifier : std::uint8_t
    {
        kNoTime = 0,
        kNotSynchronized,
        kSynchronized,
        kUnstable,
        kTimeJumpDetected,
    };

    enum class TimePointQualifier : std::uint8_t
    {
        kQM = 0,
        kASIL_B,
    };

    enum class AbsoluteAccuracyQualifier : std::uint8_t
    {
        kInaccGreaterThan24h = 0,
        kInaccLessThan24h,
        kInaccLessThan1h,
        kInaccLessThan15min,
        kInaccLessThan60s,
        kInaccLessThan10s,
        kInaccLessThan1s,
        kInaccLessThan500ms,
        kInaccLessThan100ms,
        kInaccLessThan50ms,
        kInaccLessThan10ms,
        kInaccuracyNotAvailable,
    };

    enum class AbsoluteSecurityQualifier : std::uint8_t
    {
        kNoTimeAvailable = 0,
        kNotTrustworthy,
        kTrustworthy,
        kVeryTrustworthy,
    };

    enum class SyncLogEvent : std::uint16_t
    {
        kVehicleState = 1,     // v1=acc(enum), v2=tpq(enum)
        kVehicleOffset = 2,    // v1=offset_ns, v2=path_delay_ns
        kVehiclePeerDelay = 3, // v1=path_delay_ns
        kAbsState = 100,       // v1=acc(enum), v2=sec(enum)
        kAbsUpdate = 101,      // v1=inaccuracy_ns, v2=source(0=kernel,1=ext,2=ntp)
        kAbsOffset = 102,      // v1=offset_ns_est, v2=inaccuracy_ns
    };

    namespace ipc
    {

        static constexpr std::size_t kVehicleLogCapacity = 256;
        static constexpr std::size_t kAbsLogCapacity = 256;

        struct SyncLogEntry final
        {
            std::int64_t monotonic_ns;
            std::uint16_t type;  // SyncLogEvent
            std::uint16_t flags; // reserved
            std::int64_t v1;
            std::int64_t v2;
        };

        struct SharedState final
        {
            static constexpr std::uint32_t kMagic = 0x53434F52; // 'SCOR'
            static constexpr std::uint16_t kVersion = 3;

            std::atomic<std::uint32_t> magic{kMagic};
            std::atomic<std::uint16_t> version{kVersion};
            std::atomic<std::uint16_t> reserved0{0};
            std::atomic<std::uint32_t> struct_size{static_cast<std::uint32_t>(sizeof(SharedState))};


            std::atomic<std::uint64_t> vehicle_seq{0};
            std::atomic<std::int64_t> vehicle_base_ns{0};
            std::atomic<std::int64_t> vehicle_base_mono_ns{0};

            std::atomic<AccuracyQualifier> vehicle_acc{AccuracyQualifier::kNoTime};
            std::atomic<TimePointQualifier> vehicle_tpq{TimePointQualifier::kQM};

            std::atomic<std::int64_t> vehicle_last_offset_ns{0};
            std::atomic<std::int64_t> vehicle_path_delay_ns{0};

            std::atomic<std::uint32_t> vehicle_log_head{0};
            std::atomic<std::uint32_t> vehicle_log_dropped{0};
            SyncLogEntry vehicle_log[kVehicleLogCapacity]{};

            std::atomic<std::uint64_t> abs_seq{0};
            std::atomic<std::int64_t> abs_base_utc_ns{0};
            std::atomic<std::int64_t> abs_base_mono_ns{0};

            std::atomic<AbsoluteAccuracyQualifier> abs_acc{AbsoluteAccuracyQualifier::kInaccuracyNotAvailable};
            std::atomic<AbsoluteSecurityQualifier> abs_sec{AbsoluteSecurityQualifier::kNoTimeAvailable};
            std::atomic<std::int64_t> abs_inaccuracy_ns{0};
            std::atomic<std::int64_t> abs_offset_ns_est{0};
            std::atomic<std::int64_t> abs_jitter_ns_est{0};
            std::atomic<std::int64_t> abs_last_update_mono_ns{0};
            std::atomic<std::uint8_t> abs_source{0};
            std::atomic<std::uint8_t> reserved1{0};
            std::atomic<std::uint16_t> reserved2{0};

            std::atomic<std::uint32_t> abs_log_head{0};
            std::atomic<std::uint32_t> abs_log_dropped{0};
            SyncLogEntry abs_log[kAbsLogCapacity]{};
        };

        inline void WriteVehicle(SharedState &s,
                                 std::int64_t base_vehicle_ns,
                                 std::int64_t base_mono_ns,
                                 AccuracyQualifier acc,
                                 TimePointQualifier tpq,
                                 std::int64_t last_offset_ns,
                                 std::int64_t path_delay_ns)
        {
            s.vehicle_seq.fetch_add(1, std::memory_order_acq_rel);
            s.vehicle_base_ns.store(base_vehicle_ns, std::memory_order_release);
            s.vehicle_base_mono_ns.store(base_mono_ns, std::memory_order_release);
            s.vehicle_acc.store(acc, std::memory_order_release);
            s.vehicle_tpq.store(tpq, std::memory_order_release);
            s.vehicle_last_offset_ns.store(last_offset_ns, std::memory_order_release);
            s.vehicle_path_delay_ns.store(path_delay_ns, std::memory_order_release);
            s.vehicle_seq.fetch_add(1, std::memory_order_acq_rel);
        }

        inline bool ReadVehicle(const SharedState &s,
                                std::int64_t &base_vehicle_ns,
                                std::int64_t &base_mono_ns,
                                AccuracyQualifier &acc,
                                TimePointQualifier &tpq)
        {
            for (int i = 0; i < 3; ++i)
            {
                const auto a = s.vehicle_seq.load(std::memory_order_acquire);
                if (a & 1U)
                    continue;

                base_vehicle_ns = s.vehicle_base_ns.load(std::memory_order_acquire);
                base_mono_ns = s.vehicle_base_mono_ns.load(std::memory_order_acquire);
                acc = s.vehicle_acc.load(std::memory_order_acquire);
                tpq = s.vehicle_tpq.load(std::memory_order_acquire);

                const auto b = s.vehicle_seq.load(std::memory_order_acquire);
                if (a == b)
                    return true;
            }
            return false;
        }

        inline void WriteAbsolute(SharedState &s,
                                  std::int64_t base_utc_ns,
                                  std::int64_t base_mono_ns,
                                  AbsoluteAccuracyQualifier acc,
                                  AbsoluteSecurityQualifier sec,
                                  std::int64_t inaccuracy_ns,
                                  std::int64_t offset_ns_est,
                                  std::int64_t jitter_ns_est,
                                  std::int64_t last_update_mono_ns,
                                  std::uint8_t source)
        {
            s.abs_seq.fetch_add(1, std::memory_order_acq_rel);
            s.abs_base_utc_ns.store(base_utc_ns, std::memory_order_release);
            s.abs_base_mono_ns.store(base_mono_ns, std::memory_order_release);
            s.abs_acc.store(acc, std::memory_order_release);
            s.abs_sec.store(sec, std::memory_order_release);
            s.abs_inaccuracy_ns.store(inaccuracy_ns, std::memory_order_release);
            s.abs_offset_ns_est.store(offset_ns_est, std::memory_order_release);
            s.abs_jitter_ns_est.store(jitter_ns_est, std::memory_order_release);
            s.abs_last_update_mono_ns.store(last_update_mono_ns, std::memory_order_release);
            s.abs_source.store(source, std::memory_order_release);
            s.abs_seq.fetch_add(1, std::memory_order_acq_rel);
        }

        inline bool ReadAbsolute(const SharedState &s,
                                 std::int64_t &base_utc_ns,
                                 std::int64_t &base_mono_ns,
                                 AbsoluteAccuracyQualifier &acc,
                                 AbsoluteSecurityQualifier &sec,
                                 std::int64_t &inaccuracy_ns,
                                 std::uint8_t &source)
        {
            for (int i = 0; i < 3; ++i)
            {
                const auto a = s.abs_seq.load(std::memory_order_acquire);
                if (a & 1U)
                    continue;

                base_utc_ns = s.abs_base_utc_ns.load(std::memory_order_acquire);
                base_mono_ns = s.abs_base_mono_ns.load(std::memory_order_acquire);
                acc = s.abs_acc.load(std::memory_order_acquire);
                sec = s.abs_sec.load(std::memory_order_acquire);
                inaccuracy_ns = s.abs_inaccuracy_ns.load(std::memory_order_acquire);
                source = s.abs_source.load(std::memory_order_acquire);

                const auto b = s.abs_seq.load(std::memory_order_acquire);
                if (a == b)
                    return true;
            }
            return false;
        }

        inline void LogVehicle(SharedState &s,
                               std::int64_t mono_ns,
                               SyncLogEvent type,
                               std::int64_t v1,
                               std::int64_t v2)
        {
            const auto head = s.vehicle_log_head.fetch_add(1, std::memory_order_acq_rel);
            const std::size_t idx = static_cast<std::size_t>(head % kVehicleLogCapacity);
            s.vehicle_log[idx] = SyncLogEntry{mono_ns, static_cast<std::uint16_t>(type), 0u, v1, v2};
        }

        inline void LogAbsolute(SharedState &s,
                                std::int64_t mono_ns,
                                SyncLogEvent type,
                                std::int64_t v1,
                                std::int64_t v2)
        {
            const auto head = s.abs_log_head.fetch_add(1, std::memory_order_acq_rel);
            const std::size_t idx = static_cast<std::size_t>(head % kAbsLogCapacity);
            s.abs_log[idx] = SyncLogEntry{mono_ns, static_cast<std::uint16_t>(type), 0u, v1, v2};
        }

    }
}
