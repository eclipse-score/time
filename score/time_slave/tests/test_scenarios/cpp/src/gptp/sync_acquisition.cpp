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
#include "gptp/sync_acquisition.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_slave/src/gptp/gptp_engine.h"
#include "score/ts_client/src/gptp_ipc_data.h"

#include "gptp/fake_socket.hpp"
#include "gptp/frame_builders.hpp"

namespace
{
const std::string kTargetName{"test_scenarios::gptp::sync_acquisition"};
constexpr int kPollIntervalMs{10};
constexpr int kMaxPollIterations{50};  // 50 × 10 ms = 500 ms max wait
}  // namespace

std::string SyncAcquisition::name() const
{
    return "sync_acquisition";
}

void SyncAcquisition::run(const std::string& /*input*/) const
{
    auto sock = std::make_unique<score::ts::cit::FakeSocket>();
    auto identity = std::make_unique<score::ts::cit::FakeIdentity>();
    score::ts::cit::FakeSocket* raw_sock = sock.get();

    score::ts::details::GptpEngineOptions opts;
    opts.iface_name = "lo";
    opts.pdelay_warmup_ms = 0;
    opts.pdelay_interval_ms = 50;
    opts.sync_timeout_ms = 3300;
    opts.jump_future_threshold_ns = 500'000'000LL;

    score::ts::details::GptpEngine engine{opts, std::move(sock), std::move(identity)};

    if (!engine.Initialize())
    {
        throw std::runtime_error{"GptpEngine::Initialize() failed in sync_acquisition"};
    }

    // Inject one Sync+FollowUp pair: hwts.tv_sec=1, master precise origin = 2 s.
    ::timespec hwts{1, 0};
    raw_sock->Push(score::ts::cit::MakeSyncFrame(1U), hwts);
    raw_sock->Push(score::ts::cit::MakeFollowUpFrame(1U, /*sec=*/2U, /*ns=*/0U));

    // Poll until GptpEngine processes the pair and commits the snapshot.
    bool sync_acquired{false};
    score::ts::GptpIpcData data{};
    for (int i = 0; i < kMaxPollIterations; ++i)
    {
        engine.FinalizeSnapshot();
        engine.ReadPTPSnapshot(data);
        if (data.status.is_synchronized)
        {
            sync_acquired = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
    }

    engine.Deinitialize();

    TRACING_INFO(
        kTargetName,
        std::pair{std::string{"sync_active"}, std::string{"true"}},
        std::pair{std::string{"sync_acquired"}, std::string{sync_acquired ? "true" : "false"}},
        std::pair{std::string{"is_not_timeout"}, std::string{!data.status.is_timeout ? "true" : "false"}},
        std::pair{std::string{"ptp_time_positive"}, std::string{data.ptp_assumed_time.count() > 0 ? "true" : "false"}});
}
