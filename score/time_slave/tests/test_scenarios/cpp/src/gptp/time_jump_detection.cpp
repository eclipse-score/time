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
#include "gptp/time_jump_detection.hpp"

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
const std::string kTargetName{"test_scenarios::gptp::time_jump_detection"};
// Pair 1: master time ≈ 2 s.  Pair 2: master time ≈ 3 s.
// Delta = 1 s > jump_future_threshold_ns (500 ms) → is_time_jump_future.
constexpr std::uint32_t kPair1MasterSec{2U};
constexpr std::uint32_t kPair2MasterSec{3U};
constexpr int kPollIntervalMs{10};
constexpr int kMaxPollIterations{100};  // 100 × 10 ms = 1 s max wait
}  // namespace

std::string TimeJumpDetection::name() const
{
    return "time_jump_detection";
}

void TimeJumpDetection::run(const std::string& /*input*/) const
{
    auto sock = std::make_unique<score::ts::cit::FakeSocket>();
    auto identity = std::make_unique<score::ts::cit::FakeIdentity>();
    score::ts::cit::FakeSocket* raw_sock = sock.get();

    score::ts::details::GptpEngineOptions opts;
    opts.iface_name = "lo";
    opts.pdelay_warmup_ms = 0;
    opts.pdelay_interval_ms = 50;
    opts.sync_timeout_ms = 3300;
    opts.jump_future_threshold_ns = 500'000'000LL;  // 500 ms

    score::ts::details::GptpEngine engine{opts, std::move(sock), std::move(identity)};

    if (!engine.Initialize())
    {
        throw std::runtime_error{"GptpEngine::Initialize() failed in time_jump_detection"};
    }

    // Pair 1: hwts.tv_sec=1, master precise origin = 2 s (establishes base).
    ::timespec hwts1{1, 0};
    raw_sock->Push(score::ts::cit::MakeSyncFrame(1U), hwts1);
    raw_sock->Push(score::ts::cit::MakeFollowUpFrame(1U, kPair1MasterSec, 0U));

    // Pair 2: hwts.tv_sec=2, master precise origin = 3 s.
    // Delta in master time = 1 s; delta threshold = 500 ms → is_time_jump_future.
    ::timespec hwts2{2, 0};
    raw_sock->Push(score::ts::cit::MakeSyncFrame(2U), hwts2);
    raw_sock->Push(score::ts::cit::MakeFollowUpFrame(2U, kPair2MasterSec, 0U));

    // Poll until GptpEngine processes both pairs and commits the jump flag.
    bool jump_detected{false};
    score::ts::GptpIpcData data{};
    for (int i = 0; i < kMaxPollIterations; ++i)
    {
        engine.FinalizeSnapshot();
        engine.ReadPTPSnapshot(data);
        if (data.status.is_time_jump_future)
        {
            jump_detected = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
    }

    engine.Deinitialize();

    TRACING_INFO(kTargetName,
                 std::pair{std::string{"time_jump_active"}, std::string{"true"}},
                 std::pair{std::string{"time_jump_future_detected"}, std::string{jump_detected ? "true" : "false"}},
                 std::pair{std::string{"is_not_correct"}, std::string{!data.status.is_correct ? "true" : "false"}});
}
