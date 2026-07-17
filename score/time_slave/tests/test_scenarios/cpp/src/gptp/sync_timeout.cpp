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
#include "gptp/sync_timeout.hpp"

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
const std::string kTargetName{"test_scenarios::gptp::sync_timeout"};
// Well below 3 300 ms production default — keeps the CIT fast.
constexpr int kShortTimeoutMs{100};
// Wait long enough for GptpEngine::FinalizeSnapshot() to observe the timeout.
constexpr auto kPostSyncWait = std::chrono::milliseconds{250};
constexpr int kPollIntervalMs{10};
constexpr int kMaxPollIterations{50};
}  // namespace

std::string SyncTimeout::name() const
{
    return "sync_timeout";
}

void SyncTimeout::run(const std::string& /*input*/) const
{
    auto sock = std::make_unique<score::ts::cit::FakeSocket>();
    auto identity = std::make_unique<score::ts::cit::FakeIdentity>();
    score::ts::cit::FakeSocket* raw_sock = sock.get();

    score::ts::details::GptpEngineOptions opts;
    opts.iface_name = "lo";
    opts.pdelay_warmup_ms = 0;
    opts.pdelay_interval_ms = 50;
    opts.sync_timeout_ms = kShortTimeoutMs;
    opts.jump_future_threshold_ns = 500'000'000LL;

    score::ts::details::GptpEngine engine{opts, std::move(sock), std::move(identity)};

    if (!engine.Initialize())
    {
        throw std::runtime_error{"GptpEngine::Initialize() failed in sync_timeout"};
    }

    // Phase 1: achieve synchronization with one Sync+FollowUp pair.
    ::timespec hwts{1, 0};
    raw_sock->Push(score::ts::cit::MakeSyncFrame(1U), hwts);
    raw_sock->Push(score::ts::cit::MakeFollowUpFrame(1U, /*sec=*/2U, /*ns=*/0U));

    bool initially_synchronized{false};
    score::ts::GptpIpcData data{};
    for (int i = 0; i < kMaxPollIterations; ++i)
    {
        engine.FinalizeSnapshot();
        engine.ReadPTPSnapshot(data);
        if (data.status.is_synchronized)
        {
            initially_synchronized = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
    }

    // Phase 2: stop injecting frames; wait beyond sync_timeout_ms.
    // FinalizeSnapshot() checks the wall-clock elapsed time and sets is_timeout.
    std::this_thread::sleep_for(kPostSyncWait);
    engine.FinalizeSnapshot();
    engine.ReadPTPSnapshot(data);

    engine.Deinitialize();

    TRACING_INFO(
        kTargetName,
        std::pair{std::string{"sync_timeout_active"}, std::string{"true"}},
        std::pair{std::string{"initially_synchronized"}, std::string{initially_synchronized ? "true" : "false"}},
        std::pair{std::string{"timeout_detected"}, std::string{data.status.is_timeout ? "true" : "false"}},
        std::pair{std::string{"sync_cleared"}, std::string{!data.status.is_synchronized ? "true" : "false"}});
}
