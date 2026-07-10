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
#include "verification/pipeline.hpp"
#include "verification/sync_detection.hpp"
#include "verification/time_jump_detection.hpp"
#include "verification/timeout_detection.hpp"

#include <chrono>
#include <future>
#include <stdexcept>
#include <string>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/ptp_machine/stub/factory.h"
#include "score/time_daemon/src/verification_machine/svt/factory.h"

namespace
{
const std::string kTargetName{"test_scenarios::verification::pipeline"};
}

// Verifies that PtpTimeInfo published by GPTPStubMachine flows correctly through
// SvtVerificationMachine (SynchronizationValidator → TimeoutValidator → TimeJumpsValidator)
// and emerges with the expected status fields intact.
class VerificationPipeline final : public Scenario
{
  public:
    ~VerificationPipeline() final = default;

    std::string name() const final
    {
        return "pipeline";
    }

    void run(const std::string& /*input*/) const final
    {
        auto ptp_machine = score::td::CreateGPTPStubMachine("cit_verif_ptp");
        auto verifier = score::td::CreateSvtVerificationMachine("cit_verif");

        std::promise<score::td::PtpTimeInfo> verified_promise;
        auto verified_future = verified_promise.get_future();

        // Wire PTP machine output into the verification pipeline.
        ptp_machine->SetPublishCallback([&verifier](const score::td::PtpTimeInfo& data) {
            verifier->OnMessage(data);
        });

        // Capture the first data point that exits the verification pipeline.
        verifier->SetPublishCallback([&verified_promise](const score::td::PtpTimeInfo& data) {
            try
            {
                verified_promise.set_value(data);
            }
            catch (const std::future_error&)
            {
                // Capture only the first verified data point.
            }
        });

        verifier->Init();

        if (!ptp_machine->Init())
        {
            throw std::runtime_error{"GPTPStubMachine::Init() failed"};
        }
        ptp_machine->Start();

        if (verified_future.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
        {
            ptp_machine->Stop();
            throw std::runtime_error{"Timed out waiting for verified PTP data through pipeline"};
        }

        ptp_machine->Stop();

        const auto data = verified_future.get();

        TRACING_INFO(
            kTargetName,
            std::pair{std::string{"pipeline_active"}, std::string{"true"}},
            std::pair{std::string{"is_synchronized"}, std::string{data.status.is_synchronized ? "true" : "false"}},
            std::pair{std::string{"is_correct"}, std::string{data.status.is_correct ? "true" : "false"}},
            std::pair{std::string{"is_timeout"}, std::string{data.status.is_timeout ? "true" : "false"}},
            std::pair{std::string{"ptp_time_positive"},
                      std::string{data.ptp_assumed_time.count() > 0 ? "true" : "false"}});
    }
};

ScenarioGroup::Ptr verification_scenario_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"verification",
                                                    {std::make_shared<VerificationPipeline>(),
                                                     std::make_shared<SyncDetection>(),
                                                     std::make_shared<TimeJumpDetection>(),
                                                     std::make_shared<TimeoutDetection>()},
                                                    {}}};
}
