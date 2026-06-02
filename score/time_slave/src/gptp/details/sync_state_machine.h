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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_SYNC_STATE_MACHINE_H
#define SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_SYNC_STATE_MACHINE_H

#include "score/ts_client/src/gptp_ipc_data.h"
#include "score/time_slave/src/gptp/details/ptp_types.h"

#include <atomic>
#include <cstdint>
#include <optional>

namespace score
{
namespace ts
{
namespace details
{

/// Output produced by a successful Sync+FollowUp pairing.
struct SyncResult
{
    std::int64_t master_ns{0};               ///< Grandmaster time (ns since epoch)
    std::int64_t offset_ns{0};               ///< local hw_ts − master_ns
    std::int64_t sync_mono_ns{0};            ///< CLOCK_MONOTONIC when the Sync frame was received
    score::ts::GptpIpcSyncFupData sync_fup_data{};  ///< Ready to copy into GptpIpcData (pdelay field filled by engine)
    bool is_time_jump_future{false};
    bool is_time_jump_past{false};
};

/**
 * @brief Two-step Sync / Follow_Up correlation state machine
 *        (IEEE 802.1AS slave port).
 *
 * Detects forward time jumps (> @p jump_future_threshold_ns) and backward
 * jumps.  Computes neighborRateRatio from successive Sync intervals.
 * Does NOT adjust any hardware clock; offset computation is purely
 * informational for the upstream consumer.
 *
 * Thread-safety: NOT thread-safe. All calls must come from the same thread
 * (the RxLoop thread in GptpEngine), except IsTimeout() which is atomic.
 */
class SyncStateMachine final
{
  public:
    /// @param jump_future_threshold_ns  Offset delta above which the state is
    ///        flagged as a future time jump.  Set to 0 to disable detection.
    explicit SyncStateMachine(std::int64_t jump_future_threshold_ns = 500'000'000LL) noexcept;

    /// Called when a Sync message is received (with its HW receive timestamp
    /// already stored in @p msg.recvHardwareTS).
    void OnSync(const PTPMessage& msg);

    /// Called when a FollowUp message is received.
    /// @return A SyncResult on a successful Sync+FUP pairing, std::nullopt otherwise.
    std::optional<SyncResult> OnFollowUp(const PTPMessage& msg);

    /// @return true if no valid Sync+FUP has been received for longer than
    ///         @p timeout_ns nanoseconds (monotonic).
    bool IsTimeout(std::int64_t mono_now_ns, std::int64_t timeout_ns) const;

    /// @return The latest computed neighborRateRatio (1.0 until first pair).
    double GetNeighborRateRatio() const
    {
        return neighbor_rate_ratio_;
    }

  private:
    SyncResult BuildResult(const PTPMessage& sync, const PTPMessage& fup) noexcept;

    SyncState state_{SyncState::kEmpty};
    PTPMessage last_sync_{};
    PTPMessage last_fup_{};
    std::int64_t last_master_ns_{0};
    bool has_previous_master_{false};
    std::int64_t jump_future_threshold_ns_;

    std::int64_t prev_slave_rx_ns_{0};
    std::int64_t prev_master_origin_ns_{0};
    double neighbor_rate_ratio_{1.0};

    std::atomic<std::int64_t> last_sync_mono_ns_{0};
    std::atomic<std::int64_t> created_mono_ns_;
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_DETAILS_SYNC_STATE_MACHINE_H
