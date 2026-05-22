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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_INSTRUMENT_PROBE_H
#define SCORE_TIME_SLAVE_SRC_GPTP_INSTRUMENT_PROBE_H

#include "score/time_slave/src/gptp/record/recorder.h"

#include <atomic>
#include <cstdint>

namespace score
{
namespace ts
{
namespace details
{

/// Measurement probe points within the gPTP pipeline.
enum class ProbePoint : std::uint8_t
{
    kRxPacketReceived = 0,
    kSyncFrameParsed = 1,
    kFollowUpProcessed = 2,
    kOffsetComputed = 3,
    kPdelayReqSent = 4,
    kPdelayCompleted = 5,
    kPhcAdjusted = 6,
};

/// Data payload for a single probe event.
struct ProbeData
{
    std::int64_t ts_mono_ns{0};
    std::int64_t value_ns{0};
    std::uint32_t seq_id{0};
};

/**
 * @brief Singleton manager for runtime measurement probes.
 *
 * When enabled, traces probe events to the logger and optionally to a Recorder.
 * Controlled at runtime via SetEnabled().
 */
class ProbeManager final
{
  public:
    static ProbeManager& Instance();

    void SetEnabled(bool enabled)
    {
        enabled_.store(enabled, std::memory_order_release);
    }
    bool IsEnabled() const
    {
        return enabled_.load(std::memory_order_acquire);
    }

    /// Optional: link to a Recorder for persistent probe output.
    void SetRecorder(Recorder* recorder)
    {
        recorder_.store(recorder, std::memory_order_release);
    }

    /// Record a probe event. Thread-safe.
    void Trace(ProbePoint point, const ProbeData& data);

  private:
    ProbeManager() = default;
    std::atomic<bool> enabled_{false};
    std::atomic<Recorder*> recorder_{nullptr};
};

/// Returns the current monotonic timestamp in nanoseconds.
std::int64_t ProbeMonoNs() noexcept;

}  // namespace details
}  // namespace ts
}  // namespace score

// Convenience macro: zero overhead when probing is disabled.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GPTP_PROBE(point, ...)                                                          \
    do                                                                                  \
    {                                                                                   \
        if (::score::ts::details::ProbeManager::Instance().IsEnabled())                 \
        {                                                                               \
            ::score::ts::details::ProbeManager::Instance().Trace(point, {__VA_ARGS__}); \
        }                                                                               \
    } while (0)

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_INSTRUMENT_PROBE_H
