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
#include "svt/stub_sync.hpp"

#include <chrono>
#include <future>
#include <stdexcept>
#include <string>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/ptp_machine/stub/factory.h"

namespace
{
const std::string kTargetName{"test_scenarios::svt::stub_publishes_data"};
constexpr auto kDataPublishTimeout = std::chrono::seconds{5};
}

class StubPublishesData final : public Scenario
{
  public:
    ~StubPublishesData() final = default;

    std::string name() const final
    {
        return "stub_publishes_data";
    }

    void run(const std::string& /*input*/) const final
    {
        auto machine = score::td::CreateGPTPStubMachine("cit_stub");

        std::promise<score::td::PtpTimeInfo> data_promise;
        auto data_future = data_promise.get_future();

        machine->SetPublishCallback([&data_promise](const score::td::PtpTimeInfo& data) {
            try
            {
                data_promise.set_value(data);
            }
            catch (const std::future_error&)
            {
                // Capture only the first published data point.
            }
        });

        if (!machine->Init())
        {
            throw std::runtime_error{"GPTPStubMachine::Init() failed"};
        }
        machine->Start();

        if (data_future.wait_for(kDataPublishTimeout) != std::future_status::ready)
        {
            machine->Stop();
            throw std::runtime_error{"Timed out waiting for PTP data publication"};
        }

        machine->Stop();

        const auto data = data_future.get();

        TRACING_INFO(kTargetName,
                     std::pair{std::string{"is_synchronized"},
                               std::string{data.status.is_synchronized ? "true" : "false"}},
                     std::pair{std::string{"is_correct"}, std::string{data.status.is_correct ? "true" : "false"}},
                     std::pair{std::string{"is_timeout"}, std::string{data.status.is_timeout ? "true" : "false"}},
                     std::pair{std::string{"ptp_time_positive"},
                               std::string{data.ptp_assumed_time.count() > 0 ? "true" : "false"}});
    }
};

ScenarioGroup::Ptr svt_scenario_group()
{
    return ScenarioGroup::Ptr{
        new ScenarioGroupImpl{"svt", {std::make_shared<StubPublishesData>()}, {}}};
}
