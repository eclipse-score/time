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
#include "control_flow/ptp_divider.hpp"

#include <chrono>
#include <future>
#include <stdexcept>
#include <string>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/control_flow_divider/ptp/factory.h"

namespace
{
const std::string kTargetName{"test_scenarios::control_flow::ptp_divider"};
constexpr auto kOutputPublishTimeout = std::chrono::seconds{2};
}  // namespace

// Verifies that PtpControlFlowDivider forwards a PtpTimeInfo message intact to its
// publish callback.  The divider runs on its own EventDrivenMachine worker thread, so
// a promise/future pair synchronises the test without busy-waiting.
class PtpDivider final : public Scenario
{
  public:
    ~PtpDivider() final = default;

    std::string name() const final
    {
        return "ptp_divider";
    }

    void run(const std::string& /*input*/) const final
    {
        auto divider = score::td::CreatePtpControlFlowDivider("cit_ptp_divider", std::chrono::milliseconds{100});

        std::promise<score::td::PtpTimeInfo> output_promise;
        auto output_future = output_promise.get_future();

        divider->SetPublishCallback([&output_promise](const score::td::PtpTimeInfo& out) {
            try
            {
                output_promise.set_value(out);
            }
            catch (const std::future_error&)
            {
                // Accept only the first published message.
            }
        });

        if (!divider->Init())
        {
            throw std::runtime_error{"PtpControlFlowDivider::Init() failed"};
        }
        divider->Start();

        score::td::PtpTimeInfo input{};
        input.ptp_assumed_time = std::chrono::nanoseconds{2'500'000'000};  // 2.5 s
        input.status.is_synchronized = true;
        input.status.is_correct = true;
        input.sync_fup_data.sequence_id = 1U;
        divider->OnMessage(input);

        if (output_future.wait_for(kOutputPublishTimeout) != std::future_status::ready)
        {
            divider->Stop();
            throw std::runtime_error{"Timed out waiting for PtpControlFlowDivider output"};
        }

        divider->Stop();

        const auto output = output_future.get();

        const bool ptp_time_preserved = (output.ptp_assumed_time == input.ptp_assumed_time);
        const bool sync_status_preserved = (output.status.is_synchronized == input.status.is_synchronized);

        TRACING_INFO(
            kTargetName,
            std::pair{std::string{"ptp_divider_active"}, std::string{"true"}},
            std::pair{std::string{"ptp_time_preserved"}, std::string{ptp_time_preserved ? "true" : "false"}},
            std::pair{std::string{"sync_status_preserved"}, std::string{sync_status_preserved ? "true" : "false"}});
    }
};

ScenarioGroup::Ptr control_flow_scenario_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"control_flow", {std::make_shared<PtpDivider>()}, {}}};
}
