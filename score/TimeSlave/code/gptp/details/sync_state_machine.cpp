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
#include "score/TimeSlave/code/gptp/details/sync_state_machine.h"

#include <time.h>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

std::int64_t MonoNs() noexcept
{
    ::timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::int64_t>(ts.tv_sec) * kNsPerSec + ts.tv_nsec;
}

}  // namespace

SyncStateMachine::SyncStateMachine(
    std::int64_t jump_future_threshold_ns) noexcept
    : jump_future_threshold_ns_{jump_future_threshold_ns}
{
}

void SyncStateMachine::OnSync(const PTPMessage& msg)
{
    switch (state_)
    {
        case SyncState::kEmpty:
            last_sync_ = msg;
            state_     = SyncState::kHaveSync;
            break;

        case SyncState::kHaveSync:
            // Newer Sync replaces the stale one (master sends faster than FUP arrives)
            last_sync_ = msg;
            break;

        case SyncState::kHaveFup:
            // Buffered FUP is now stale; start fresh with the new Sync
            last_sync_ = msg;
            state_     = SyncState::kHaveSync;
            break;
    }
}

std::optional<SyncResult> SyncStateMachine::OnFollowUp(
    const PTPMessage& msg)
{
    switch (state_)
    {
        case SyncState::kEmpty:
            // FUP arrived before its Sync — buffer it and wait
            last_fup_ = msg;
            state_    = SyncState::kHaveFup;
            return std::nullopt;

        case SyncState::kHaveFup:
            // Another FUP without a matching Sync — replace buffer
            last_fup_ = msg;
            return std::nullopt;

        case SyncState::kHaveSync:
            if (last_sync_.ptpHdr.sequenceId != msg.ptpHdr.sequenceId)
            {
                // Sequence-ID mismatch: buffer the FUP and wait for matching Sync
                last_fup_ = msg;
                state_    = SyncState::kHaveFup;
                return std::nullopt;
            }

            {
                SyncResult result = BuildResult(last_sync_, msg);
                state_ = SyncState::kEmpty;
                last_sync_mono_ns_.store(MonoNs(), std::memory_order_release);
                return result;
            }
    }
    return std::nullopt;
}

bool SyncStateMachine::IsTimeout(std::int64_t mono_now_ns,
                                        std::int64_t timeout_ns) const
{
    if (timeout_ns <= 0)
        return false;
    const std::int64_t last = last_sync_mono_ns_.load(std::memory_order_acquire);
    if (last == 0)
        return false;   // never synchronized yet — not a "timeout"
    return (mono_now_ns - last) > timeout_ns;
}

SyncResult SyncStateMachine::BuildResult(
    const PTPMessage& sync,
    const PTPMessage& fup) noexcept
{
    const TmvT sync_corr = CorrectionToTmv(sync.ptpHdr.correctionField);
    const TmvT fup_corr  = CorrectionToTmv(fup.ptpHdr.correctionField);
    const TmvT fup_ts    = TimestampToTmv(fup.follow_up.preciseOriginTimestamp);

    const std::int64_t master_ns = fup_ts.ns + sync_corr.ns + fup_corr.ns;
    const std::int64_t offset_ns = sync.recvHardwareTS.ns - master_ns;

    SyncResult r{};
    r.master_ns = master_ns;
    r.offset_ns = offset_ns;

    if (last_master_ns_ != 0)
    {
        const std::int64_t delta = master_ns - last_master_ns_;
        if (delta < 0)
            r.is_time_jump_past = true;
        else if (jump_future_threshold_ns_ > 0 && delta > jump_future_threshold_ns_)
            r.is_time_jump_future = true;
    }

    score::td::SyncFupData& d  = r.sync_fup_data;
    d.precise_origin_timestamp =
        static_cast<std::uint64_t>(fup_ts.ns);
    d.reference_global_timestamp =
        static_cast<std::uint64_t>(master_ns);
    d.reference_local_timestamp =
        static_cast<std::uint64_t>(sync.recvHardwareTS.ns);
    d.sync_ingress_timestamp =
        static_cast<std::uint64_t>(sync.recvHardwareTS.ns);
    d.correction_field =
        static_cast<std::uint64_t>(sync.ptpHdr.correctionField);
    d.sequence_id  = fup.ptpHdr.sequenceId;
    d.pdelay       = 0U;  // filled by GptpEngine from IPeerDelayMeasurer
    d.port_number  = sync.ptpHdr.sourcePortIdentity.portNumber;
    d.clock_identity =
        ClockIdentityToU64(sync.ptpHdr.sourcePortIdentity.clockIdentity);

    // IEEE 802.1AS Clause 11.4.1
    if (prev_slave_rx_ns_ != 0 && prev_master_origin_ns_ != 0)
    {
        const std::int64_t slave_interval  = sync.recvHardwareTS.ns - prev_slave_rx_ns_;
        const std::int64_t master_interval = master_ns - prev_master_origin_ns_;
        if (master_interval > 0)
        {
            neighbor_rate_ratio_ =
                static_cast<double>(slave_interval) / static_cast<double>(master_interval);
        }
    }
    prev_slave_rx_ns_      = sync.recvHardwareTS.ns;
    prev_master_origin_ns_ = master_ns;

    last_master_ns_ = master_ns;

    return r;
}

}  // namespace details
}  // namespace ts
}  // namespace score
