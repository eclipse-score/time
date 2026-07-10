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
#include "verification/sync_detection.hpp"

#include <chrono>
#include <string>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/verification_machine/svt/factory.h"

namespace
{
const std::string kTargetName{"test_scenarios::verification::sync_detection"};

// Constructs a minimal PtpTimeInfo with the given synchronization state.
// Unique sequence_id values ensure TimeoutValidator always sees a "new frame".
score::td::PtpTimeInfo make_ptp_data(bool synchronized, std::uint16_t seq_id)
{
    score::td::PtpTimeInfo data{};
    data.ptp_assumed_time = std::chrono::nanoseconds{1'000'000'000};
    data.status.is_synchronized = synchronized;
    data.status.is_correct = synchronized;
    data.sync_fup_data.sequence_id = seq_id;
    return data;
}
}  // namespace

std::string SyncDetection::name() const
{
    return "sync_detection";
}

void SyncDetection::run(const std::string& /*input*/) const
{
    auto verifier = score::td::CreateSvtVerificationMachine("cit_sync_detect");
    verifier->Init();

    // --- Step 1: inject unsynchronized data ---
    // SynchronizationValidator has not yet latched; is_synchronized must stay false.
    bool unsync_output_is_synchronized{true};  // intentionally wrong initial value
    verifier->SetPublishCallback([&unsync_output_is_synchronized](const score::td::PtpTimeInfo& out) {
        unsync_output_is_synchronized = out.status.is_synchronized;
    });
    verifier->OnMessage(make_ptp_data(false, 1U));

    // --- Step 2: inject synchronized data ---
    // SynchronizationValidator latches internally; output must be is_synchronized=true.
    bool sync_output_is_synchronized{false};
    verifier->SetPublishCallback([&sync_output_is_synchronized](const score::td::PtpTimeInfo& out) {
        sync_output_is_synchronized = out.status.is_synchronized;
    });
    verifier->OnMessage(make_ptp_data(true, 2U));

    // --- Step 3: inject unsynchronized data again ---
    // Latch must hold: SynchronizationValidator forces is_synchronized=true even for
    // subsequent messages with is_synchronized=false in the input.
    bool latch_output_is_synchronized{false};
    verifier->SetPublishCallback([&latch_output_is_synchronized](const score::td::PtpTimeInfo& out) {
        latch_output_is_synchronized = out.status.is_synchronized;
    });
    verifier->OnMessage(make_ptp_data(false, 3U));

    TRACING_INFO(kTargetName,
                 std::pair{std::string{"sync_detection_active"}, std::string{"true"}},
                 std::pair{std::string{"unsync_output_is_synchronized"},
                           std::string{unsync_output_is_synchronized ? "true" : "false"}},
                 std::pair{std::string{"sync_output_is_synchronized"},
                           std::string{sync_output_is_synchronized ? "true" : "false"}},
                 std::pair{std::string{"latch_output_is_synchronized"},
                           std::string{latch_output_is_synchronized ? "true" : "false"}});
}
