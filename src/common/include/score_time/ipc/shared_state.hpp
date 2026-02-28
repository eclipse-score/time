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
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace score_time
{

    /**
     * @brief Vehicle time accuracy qualifier
     *
     * Indicates the synchronization status and reliability of vehicle time
     * obtained via gPTP (IEEE 802.1AS). Used by clients to determine if
     * the time is suitable for safety-critical operations.
     */
    enum class AccuracyQualifier : std::uint8_t
    {
        kNoTime = 0,           ///< No time available yet
        kNotSynchronized,      ///< Time available but not synchronized to grandmaster
        kSynchronized,         ///< Successfully synchronized to gPTP grandmaster
        kUnstable,             ///< Synchronization unstable or degraded
        kTimeJumpDetected,     ///< Discontinuous time change detected
    };

    /**
     * @brief ASIL (Automotive Safety Integrity Level) qualifier for time point
     *
     * Indicates the safety integrity level of the time synchronization.
     * Complies with ISO 26262 functional safety standard.
     */
    enum class TimePointQualifier : std::uint8_t
    {
        kQM = 0,     ///< Quality Management (non-safety-critical)
        kASIL_B,     ///< ASIL-B safety integrity level
    };

    /**
     * @brief Absolute (UTC/wall-clock) time accuracy qualifier
     *
     * Indicates the estimated inaccuracy range of absolute time
     * obtained from external sources (GNSS, NTP, kernel RTC).
     * Corresponds to AUTOSAR Absolute Time Accuracy levels.
     */
    enum class AbsoluteAccuracyQualifier : std::uint8_t
    {
        kInaccGreaterThan24h = 0,  ///< Inaccuracy > 24 hours
        kInaccLessThan24h,         ///< Inaccuracy < 24 hours
        kInaccLessThan1h,          ///< Inaccuracy < 1 hour
        kInaccLessThan15min,       ///< Inaccuracy < 15 minutes
        kInaccLessThan60s,         ///< Inaccuracy < 60 seconds
        kInaccLessThan10s,         ///< Inaccuracy < 10 seconds
        kInaccLessThan1s,          ///< Inaccuracy < 1 second
        kInaccLessThan500ms,       ///< Inaccuracy < 500 milliseconds
        kInaccLessThan100ms,       ///< Inaccuracy < 100 milliseconds
        kInaccLessThan50ms,        ///< Inaccuracy < 50 milliseconds
        kInaccLessThan10ms,        ///< Inaccuracy < 10 milliseconds
        kInaccuracyNotAvailable,   ///< Inaccuracy information not available
    };

    /**
     * @brief Absolute time security/trustworthiness qualifier
     *
     * Indicates the trustworthiness of the absolute time source.
     * Higher values indicate more secure/authenticated time sources
     * (e.g., cryptographically signed GNSS vs. unauthenticated NTP).
     */
    enum class AbsoluteSecurityQualifier : std::uint8_t
    {
        kNoTimeAvailable = 0,  ///< No absolute time available
        kNotTrustworthy,       ///< Source is not trustworthy (unauthenticated)
        kTrustworthy,          ///< Source is trustworthy (basic validation)
        kVeryTrustworthy,      ///< Source is highly trustworthy (authenticated/signed)
    };

    /**
     * @brief Synchronization log event types
     *
     * Defines event types stored in the circular log buffers (vehicle_log and abs_log).
     * Each event has associated data in v1 and v2 fields of SyncLogEntry.
     */
    enum class SyncLogEvent : std::uint16_t
    {
        kVehicleState = 1,     ///< Vehicle time state change (v1=acc enum, v2=tpq enum)
        kVehicleOffset = 2,    ///< Vehicle time offset update (v1=offset_ns, v2=path_delay_ns)
        kVehiclePeerDelay = 3, ///< gPTP peer delay measurement (v1=path_delay_ns)
        kAbsState = 100,       ///< Absolute time state change (v1=acc enum, v2=sec enum)
        kAbsUpdate = 101,      ///< Absolute time update (v1=inaccuracy_ns, v2=source: 0=kernel, 1=external, 2=NTP)
        kAbsOffset = 102,      ///< Absolute time offset estimate (v1=offset_ns_est, v2=inaccuracy_ns)
    };

    namespace ipc
    {

        static constexpr std::size_t kVehicleLogCapacity = 256;
        static constexpr std::size_t kAbsLogCapacity = 256;

        /**
         * @brief Single entry in the synchronization event log
         *
         * Uses per-entry seqlock protocol for lock-free concurrent access between
         * writer (tsyncd daemon) and readers (client applications).
         *
         * **Seqlock Protocol:**
         * - seq is even (0, 2, 4, ...) when entry is readable
         * - seq is odd (1, 3, 5, ...) when writer is updating the entry
         * - Readers retry if seq is odd or changes during read
         *
         * All fields are atomic to prevent torn reads on architectures where
         * 64-bit loads are not naturally atomic.
         */
        struct SyncLogEntry final
        {
            std::atomic<std::uint64_t> seq{0};         ///< Seqlock: even=readable, odd=writing
            std::atomic<std::int64_t> monotonic_ns{0}; ///< Monotonic timestamp of event (CLOCK_MONOTONIC)
            std::atomic<std::uint16_t> type{0};        ///< Event type (SyncLogEvent)
            std::atomic<std::uint16_t> flags{0};       ///< Reserved for future use
            std::atomic<std::int64_t> v1{0};           ///< Event-specific data field 1 (see SyncLogEvent)
            std::atomic<std::int64_t> v2{0};           ///< Event-specific data field 2 (see SyncLogEvent)
        };

        /**
         * @brief Main shared memory structure for IPC between tsyncd daemon and clients
         *
         * This structure is mapped into shared memory (typically /dev/shm/score_time) and
         * provides lock-free access to synchronized time information using seqlock protocol.
         *
         * **Memory Layout:**
         * - Header fields (magic, version, size) for validation
         * - Vehicle time data (gPTP/IEEE 802.1AS synchronized time)
         * - Absolute time data (UTC/wall-clock time from GNSS/NTP)
         * - Circular event logs for diagnostics
         *
         * **Concurrency:**
         * - Single writer (tsyncd daemon)
         * - Multiple readers (client applications)
         * - Uses double-buffering seqlock for vehicle/absolute time
         * - Uses per-entry seqlock for log entries
         *
         * **Version History:**
         * - v1-v3: Original implementations
         * - v4: Added per-entry seqlock to prevent torn reads in logs
         *
         * @note All atomic fields use appropriate memory_order for lock-free correctness
         * @warning Clients must check magic and version before accessing other fields
         */
        struct SharedState final
        {
            static constexpr std::uint32_t kMagic = 0x53434F52; ///< Magic number 'SCOR' for validation
            static constexpr std::uint16_t kVersion = 4;        ///< Structure version (incremented on ABI break)

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

        /**
         * @brief Write vehicle time to shared memory using seqlock protocol
         *
         * Called by tsyncd daemon to update vehicle time after gPTP synchronization.
         * Uses seqlock protocol to allow lock-free reads by multiple clients.
         *
         * **Seqlock Protocol:**
         * 1. Increment vehicle_seq (odd = writing in progress)
         * 2. Write all vehicle time fields
         * 3. Increment vehicle_seq again (even = read complete)
         *
         * @param s Reference to SharedState in shared memory
         * @param base_vehicle_ns Vehicle time base in nanoseconds (gPTP synchronized time)
         * @param base_mono_ns Monotonic time base in nanoseconds (CLOCK_MONOTONIC)
         * @param acc Accuracy qualifier indicating synchronization status
         * @param tpq Time point qualifier indicating ASIL level
         * @param last_offset_ns Last measured offset from grandmaster in nanoseconds
         * @param path_delay_ns Peer-to-peer delay in nanoseconds
         *
         * @note This function is noexcept and cannot fail
         * @note Only tsyncd daemon should call this function (single writer assumption)
         */
        inline void WriteVehicle(SharedState &s,
                                 std::int64_t base_vehicle_ns,
                                 std::int64_t base_mono_ns,
                                 AccuracyQualifier acc,
                                 TimePointQualifier tpq,
                                 std::int64_t last_offset_ns,
                                 std::int64_t path_delay_ns) noexcept
        {
            s.vehicle_seq.fetch_add(1, std::memory_order_release);
            s.vehicle_base_ns.store(base_vehicle_ns, std::memory_order_release);
            s.vehicle_base_mono_ns.store(base_mono_ns, std::memory_order_release);
            s.vehicle_acc.store(acc, std::memory_order_release);
            s.vehicle_tpq.store(tpq, std::memory_order_release);
            s.vehicle_last_offset_ns.store(last_offset_ns, std::memory_order_release);
            s.vehicle_path_delay_ns.store(path_delay_ns, std::memory_order_release);
            s.vehicle_seq.fetch_add(1, std::memory_order_release);  // Fixed: was acq_rel
        }

        /**
         * @brief Read vehicle time from shared memory using seqlock protocol
         *
         * Called by client applications to read vehicle time in a lock-free manner.
         * Uses seqlock protocol with retry to handle concurrent writes.
         *
         * **Seqlock Read Protocol:**
         * 1. Read vehicle_seq (must be even)
         * 2. Read all vehicle time fields
         * 3. Re-read vehicle_seq
         * 4. If seq changed or was odd, retry (up to 1000 attempts)
         *
         * @param s Const reference to SharedState in shared memory
         * @param[out] base_vehicle_ns Vehicle time base in nanoseconds
         * @param[out] base_mono_ns Monotonic time base in nanoseconds
         * @param[out] acc Accuracy qualifier
         * @param[out] tpq Time point qualifier
         * @return true if read successful, false if exceeded retry limit
         *
         * @note Returns false only under extreme contention (unlikely in practice)
         * @note Uses 1000 retry limit for safety-critical system resilience
         */
        inline bool ReadVehicle(const SharedState &s,
                                std::int64_t &base_vehicle_ns,
                                std::int64_t &base_mono_ns,
                                AccuracyQualifier &acc,
                                TimePointQualifier &tpq)
        {
            // Higher retry limit for safety-critical systems - 1000 attempts
            // provides resilience under high contention
            for (int i = 0; i < 1000; ++i)
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

        /**
         * @brief Write absolute (UTC) time to shared memory using seqlock protocol
         *
         * Called by tsyncd daemon to update absolute time from external sources
         * (GNSS, NTP, kernel RTC). Uses same seqlock protocol as WriteVehicle.
         *
         * @param s Reference to SharedState in shared memory
         * @param base_utc_ns UTC time base in nanoseconds since Unix epoch
         * @param base_mono_ns Monotonic time base in nanoseconds (CLOCK_MONOTONIC)
         * @param acc Absolute accuracy qualifier
         * @param sec Security/trustworthiness qualifier
         * @param inaccuracy_ns Estimated inaccuracy in nanoseconds
         * @param offset_ns_est Estimated offset from true UTC in nanoseconds
         * @param jitter_ns_est Estimated jitter in nanoseconds
         * @param last_update_mono_ns Monotonic timestamp of last update
         * @param source Time source: 0=kernel, 1=external/GNSS, 2=NTP
         *
         * @note This function is noexcept and cannot fail
         * @note Only tsyncd daemon should call this function (single writer assumption)
         */
        inline void WriteAbsolute(SharedState &s,
                                  std::int64_t base_utc_ns,
                                  std::int64_t base_mono_ns,
                                  AbsoluteAccuracyQualifier acc,
                                  AbsoluteSecurityQualifier sec,
                                  std::int64_t inaccuracy_ns,
                                  std::int64_t offset_ns_est,
                                  std::int64_t jitter_ns_est,
                                  std::int64_t last_update_mono_ns,
                                  std::uint8_t source) noexcept
        {
            s.abs_seq.fetch_add(1, std::memory_order_release);
            s.abs_base_utc_ns.store(base_utc_ns, std::memory_order_release);
            s.abs_base_mono_ns.store(base_mono_ns, std::memory_order_release);
            s.abs_acc.store(acc, std::memory_order_release);
            s.abs_sec.store(sec, std::memory_order_release);
            s.abs_inaccuracy_ns.store(inaccuracy_ns, std::memory_order_release);
            s.abs_offset_ns_est.store(offset_ns_est, std::memory_order_release);
            s.abs_jitter_ns_est.store(jitter_ns_est, std::memory_order_release);
            s.abs_last_update_mono_ns.store(last_update_mono_ns, std::memory_order_release);
            s.abs_source.store(source, std::memory_order_release);
            s.abs_seq.fetch_add(1, std::memory_order_release);  // Fixed: was acq_rel
        }

        /**
         * @brief Read absolute (UTC) time from shared memory using seqlock protocol
         *
         * Called by client applications to read absolute time in a lock-free manner.
         * Uses same seqlock read protocol as ReadVehicle with 1000 retry limit.
         *
         * @param s Const reference to SharedState in shared memory
         * @param[out] base_utc_ns UTC time base in nanoseconds since Unix epoch
         * @param[out] base_mono_ns Monotonic time base in nanoseconds
         * @param[out] acc Absolute accuracy qualifier
         * @param[out] sec Security/trustworthiness qualifier
         * @param[out] inaccuracy_ns Estimated inaccuracy in nanoseconds
         * @param[out] source Time source (0=kernel, 1=external, 2=NTP)
         * @return true if read successful, false if exceeded retry limit
         *
         * @note Returns false only under extreme contention (unlikely in practice)
         */
        inline bool ReadAbsolute(const SharedState &s,
                                 std::int64_t &base_utc_ns,
                                 std::int64_t &base_mono_ns,
                                 AbsoluteAccuracyQualifier &acc,
                                 AbsoluteSecurityQualifier &sec,
                                 std::int64_t &inaccuracy_ns,
                                 std::uint8_t &source)
        {
            // Higher retry limit for safety-critical systems - 1000 attempts
            // provides resilience under high contention
            for (int i = 0; i < 1000; ++i)
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

            auto &entry = s.vehicle_log[idx];
            entry.seq.fetch_add(1, std::memory_order_release);  // Odd = writing (发布"正在写入"标志)
            entry.monotonic_ns.store(mono_ns, std::memory_order_relaxed);
            entry.type.store(static_cast<std::uint16_t>(type), std::memory_order_relaxed);
            entry.flags.store(0u, std::memory_order_relaxed);
            entry.v1.store(v1, std::memory_order_relaxed);
            entry.v2.store(v2, std::memory_order_relaxed);
            entry.seq.fetch_add(1, std::memory_order_release);  // Even = complete (发布写入完成)
        }

        inline void LogAbsolute(SharedState &s,
                                std::int64_t mono_ns,
                                SyncLogEvent type,
                                std::int64_t v1,
                                std::int64_t v2)
        {
            const auto head = s.abs_log_head.fetch_add(1, std::memory_order_acq_rel);
            const std::size_t idx = static_cast<std::size_t>(head % kAbsLogCapacity);

            auto &entry = s.abs_log[idx];
            entry.seq.fetch_add(1, std::memory_order_release);  // Odd = writing (发布"正在写入"标志)
            entry.monotonic_ns.store(mono_ns, std::memory_order_relaxed);
            entry.type.store(static_cast<std::uint16_t>(type), std::memory_order_relaxed);
            entry.flags.store(0u, std::memory_order_relaxed);
            entry.v1.store(v1, std::memory_order_relaxed);
            entry.v2.store(v2, std::memory_order_relaxed);
            entry.seq.fetch_add(1, std::memory_order_release);  // Even = complete (发布写入完成)
        }

    }
}
