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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_RECORD_RECORDER_H
#define SCORE_TIME_SLAVE_SRC_GPTP_RECORD_RECORDER_H

#include <atomic>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/// Event types that can be recorded.
enum class RecordEvent : std::uint8_t
{
    kSyncReceived = 0,
    kPdelayCompleted = 1,
    kClockJump = 2,
    kOffsetThreshold = 3,
    kProbe = 4,
};

/// A single record entry written to the log file.
struct RecordEntry
{
    std::int64_t mono_ns{0};
    RecordEvent event{RecordEvent::kSyncReceived};
    std::int64_t offset_ns{0};
    std::int64_t pdelay_ns{0};
    std::uint16_t seq_id{0};
    std::uint8_t status_flags{0};
};

/**
 * @brief Thread-safe CSV file recorder for gPTP events.
 *
 * When enabled, appends CSV lines to the configured file path.
 * Format: mono_ns,event,offset_ns,pdelay_ns,seq_id,status_flags
 */
class Recorder final
{
  public:
    struct Config
    {
        bool enabled = false;
        std::string file_path = "/var/log/gptp_record.csv";
        std::int64_t offset_threshold_ns = 1'000'000LL;  ///< 1 ms
        std::uint32_t flush_interval = 8U;
    };

    explicit Recorder(Config cfg);
    ~Recorder() = default;

    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    bool IsEnabled() const
    {
        return enabled_.load(std::memory_order_relaxed) && file_.is_open();
    }

    /// Record an entry. Thread-safe.
    void Record(const RecordEntry& entry);

  private:
    Config cfg_;
    std::atomic<bool> enabled_{false};
    std::mutex mutex_;
    std::ofstream file_;
    std::uint32_t flush_counter_{0U};
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_RECORD_RECORDER_H
