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
#include "verification/time_jump_detection.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/verification_machine/svt/factory.h"

namespace
{
const std::string kTargetName{"test_scenarios::verification::time_jump_detection"};
constexpr auto kSyncDebounceWait = std::chrono::milliseconds{5500};

// Base timestamps for warmup frames.  All warmup frames advance both
// sync_ingress_timestamp and precise_origin_timestamp by the same 125 µs step
// so IsTimeJumpDetected sees no jump between consecutive warmup frames.
constexpr std::uint64_t kBaseIngress{1'000'000'000ULL};        // 1 s
constexpr std::uint64_t kBaseOrigin{2'000'000'000ULL};         // 2 s
constexpr std::uint64_t kFrameStep{125'000ULL};                 // 125 µs per frame

// Constructs a PtpTimeInfo with consistent fields for the debouncing warmup phase.
// Both sync_ingress_timestamp and precise_origin_timestamp advance by the same step
// so no jump is detected between consecutive warmup frames.
score::td::PtpTimeInfo make_warmup_frame(std::uint16_t seq_id)
{
    score::td::PtpTimeInfo data{};
    data.ptp_assumed_time                          = std::chrono::nanoseconds{1'000'000'000};
    data.status.is_synchronized                    = true;
    data.status.is_correct                         = true;
    data.sync_fup_data.sequence_id                 = seq_id;
    // Monotonically advancing timestamps — no jump between frames.
    data.sync_fup_data.sync_ingress_timestamp      = kBaseIngress + static_cast<std::uint64_t>(seq_id) * kFrameStep;
    data.sync_fup_data.precise_origin_timestamp    = kBaseOrigin  + static_cast<std::uint64_t>(seq_id) * kFrameStep;
    return data;
}

// Constructs a PtpTimeInfo where precise_origin_timestamp jumps 1 ms ahead of the
// value predicted from the ingress delta — 1 ms > max_time_jump_allowed (500 µs)
// triggers IsTimeJumpDetected → kJumpToFuture → is_time_jump_future=true.
score::td::PtpTimeInfo make_jump_frame(std::uint16_t seq_id)
{
    score::td::PtpTimeInfo data{};
    data.ptp_assumed_time                       = std::chrono::nanoseconds{1'001'000'000};  // 1 s + 1 ms
    data.status.is_synchronized                 = true;
    data.status.is_correct                      = true;
    data.sync_fup_data.sequence_id              = seq_id;
    // ingress advances normally; origin jumps 1 ms forward vs predicted → jump-to-future
    data.sync_fup_data.sync_ingress_timestamp   = kBaseIngress + static_cast<std::uint64_t>(seq_id) * kFrameStep;
    data.sync_fup_data.precise_origin_timestamp = kBaseOrigin
        + static_cast<std::uint64_t>(seq_id) * kFrameStep
        + 1'000'000ULL;  // +1 ms extra on origin side
    return data;
}
}  // namespace

std::string TimeJumpDetection::name() const
{
    return "time_jump_detection";
}

void TimeJumpDetection::run(const std::string& /*input*/) const
{
    auto verifier = score::td::CreateSvtVerificationMachine("cit_time_jump");
    verifier->Init();

    // --- Phase 1: prime the SynchronizationValidator latch and start debouncing ---
    // The first sync=true frame latches SynchronizationValidator (all future messages
    // will exit stage 1 with is_synchronized=true) and moves TimeJumpsValidator from
    // kIdle into kInitialSyncDebouncing.
    auto noop = [](const score::td::PtpTimeInfo&) {};
    verifier->SetPublishCallback(noop);
    verifier->OnMessage(make_warmup_frame(1U));

    // --- Phase 2: wait for the real-clock debounce threshold (5 000 000 000 ns) ---
    // TimeJumpsValidator stays in kInitialSyncDebouncing until 5 s of wall-clock
    // time has elapsed from when is_synchronized was first observed.
    std::this_thread::sleep_for(kSyncDebounceWait);

    // --- Phase 3: send enough sync frames to satisfy valid_frames_threshold (2) ---
    // Frames 2-4 complete the frame count and also trigger the transition check on
    // each OnMessage() call, so at least one of them exits kInitialSyncDebouncing.
    for (std::uint16_t i = 2U; i <= 4U; ++i)
    {
        verifier->SetPublishCallback(noop);
        verifier->OnMessage(make_warmup_frame(i));
    }

    // --- Phase 4: inject a forward time jump > max_time_jump_allowed (500 µs) ---
    bool time_jump_future_detected{false};
    verifier->SetPublishCallback([&time_jump_future_detected](const score::td::PtpTimeInfo& out) {
        time_jump_future_detected = out.status.is_time_jump_future;
    });
    verifier->OnMessage(make_jump_frame(5U));

    TRACING_INFO(kTargetName,
                 std::pair{std::string{"time_jump_detection_active"}, std::string{"true"}},
                 std::pair{std::string{"time_jump_future_detected"},
                           std::string{time_jump_future_detected ? "true" : "false"}});
}
