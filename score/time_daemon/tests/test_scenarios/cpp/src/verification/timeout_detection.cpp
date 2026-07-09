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
#include "verification/timeout_detection.hpp"

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
const std::string kTargetName{"test_scenarios::verification::timeout_detection"};

// Slightly above the 3 300 000 000 ns threshold hardcoded in CreateSvtVerificationMachine.
constexpr auto kTimeoutThresholdWait = std::chrono::milliseconds{3600};

score::td::PtpTimeInfo make_ptp_frame(std::uint16_t seq_id)
{
    score::td::PtpTimeInfo data{};
    data.ptp_assumed_time          = std::chrono::nanoseconds{1'000'000'000};
    data.status.is_synchronized    = true;
    data.status.is_correct         = true;
    data.sync_fup_data.sequence_id = seq_id;
    return data;
}
}  // namespace

std::string TimeoutDetection::name() const
{
    return "timeout_detection";
}

void TimeoutDetection::run(const std::string& /*input*/) const
{
    auto verifier = score::td::CreateSvtVerificationMachine("cit_timeout_detect");
    verifier->Init();

    // --- Phase 1: first frame (seq=1) ---
    // TimeoutValidator has no prior data; treats this as a new frame.
    // reception_time_ is set to now; is_timeout must be false.
    bool phase1_is_timeout{true};  // intentionally wrong initial value
    verifier->SetPublishCallback([&phase1_is_timeout](const score::td::PtpTimeInfo& out) {
        phase1_is_timeout = out.status.is_timeout;
    });
    verifier->OnMessage(make_ptp_frame(1U));

    // --- Phase 2: wait beyond the 3 300 000 000 ns reception threshold ---
    std::this_thread::sleep_for(kTimeoutThresholdWait);

    // --- Phase 3: repeat the same frame (seq=1) ---
    // No new frame detected (same sequence_id); elapsed time > threshold.
    // TimeoutValidator must set is_timeout = true.
    bool phase3_is_timeout{false};
    verifier->SetPublishCallback([&phase3_is_timeout](const score::td::PtpTimeInfo& out) {
        phase3_is_timeout = out.status.is_timeout;
    });
    verifier->OnMessage(make_ptp_frame(1U));

    // --- Phase 4: new frame (seq=2) ---
    // TimeoutValidator updates reception_time_ and resets is_timeout = false.
    bool phase4_is_timeout{true};
    verifier->SetPublishCallback([&phase4_is_timeout](const score::td::PtpTimeInfo& out) {
        phase4_is_timeout = out.status.is_timeout;
    });
    verifier->OnMessage(make_ptp_frame(2U));

    TRACING_INFO(kTargetName,
                 std::pair{std::string{"timeout_detection_active"}, std::string{"true"}},
                 std::pair{std::string{"first_frame_not_timeout"},
                           std::string{!phase1_is_timeout ? "true" : "false"}},
                 std::pair{std::string{"timeout_detected"},
                           std::string{phase3_is_timeout ? "true" : "false"}},
                 std::pair{std::string{"timeout_cleared_on_new_frame"},
                           std::string{!phase4_is_timeout ? "true" : "false"}});
}
