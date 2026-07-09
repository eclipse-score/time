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
#include "ipc/shm_roundtrip.hpp"

#include <chrono>
#include <stdexcept>
#include <string>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/ipc/svt/publisher/factory.h"
#include "score/time_daemon/src/ipc/svt/receiver/factory.h"

namespace
{
const std::string kTargetName{"test_scenarios::ipc::shm_roundtrip"};
}

class ShmRoundtrip final : public Scenario
{
  public:
    ~ShmRoundtrip() final = default;

    std::string name() const final
    {
        return "shm_roundtrip";
    }

    void run(const std::string& /*input*/) const final
    {
        auto publisher = score::td::CreateSvtPublisher("cit_ipc_pub");
        auto receiver  = score::td::CreateSvtReceiver();

        if (!publisher->Init())
        {
            throw std::runtime_error{"SvtPublisher::Init() failed"};
        }
        if (!receiver->Init())
        {
            throw std::runtime_error{"SvtReceiver::Init() failed"};
        }

        score::td::PtpTimeInfo info{};
        info.ptp_assumed_time = std::chrono::nanoseconds{1'000'000'000LL};
        info.local_time =
            score::td::PtpTimeInfo::ReferenceClock::time_point{std::chrono::nanoseconds{500'000'000LL}};
        info.rate_deviation    = 0.0;
        info.status            = {true, false, false, false, true};
        info.sync_fup_data.sequence_id              = 42U;
        info.sync_fup_data.precise_origin_timestamp = 1'000'000'000ULL;

        publisher->OnMessage(info);

        const auto result  = receiver->Receive();
        const bool read_ok = result.has_value();

        bool ptp_time_ok = false;
        bool status_ok   = false;
        bool seq_ok      = false;

        if (read_ok)
        {
            const auto& snap = result.value();
            ptp_time_ok      = snap.ptp_assumed_time == static_cast<uint64_t>(1'000'000'000LL);
            status_ok        = snap.status.is_synchronized && snap.status.is_correct;
            seq_ok           = snap.sync_fup_data.sequence_id == 42U;
        }

        TRACING_INFO(kTargetName,
                     std::pair{std::string{"read_succeeded"}, std::string{read_ok ? "true" : "false"}},
                     std::pair{std::string{"ptp_time_preserved"}, std::string{ptp_time_ok ? "true" : "false"}},
                     std::pair{std::string{"status_preserved"}, std::string{status_ok ? "true" : "false"}},
                     std::pair{std::string{"seq_id_preserved"}, std::string{seq_ok ? "true" : "false"}});
    }
};

ScenarioGroup::Ptr ipc_scenario_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"ipc", {std::make_shared<ShmRoundtrip>()}, {}}};
}
