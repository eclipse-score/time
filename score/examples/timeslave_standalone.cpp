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

/**
 * @brief Standalone TimeSlave — bypasses the lifecycle framework.
 *
 * Directly calls GptpEngine + GptpIpcPublisher without any lifecycle manager,
 * suitable for board-level functional verification.
 *
 * Usage:
 *   ./timeslave_standalone [interface]
 *   ./timeslave_standalone emac0
 *
 * Defaults to "emac0" if no argument is given.
 */

#include "score/TimeSlave/code/gptp/gptp_engine.h"
#include "score/libTSClient/gptp_ipc_publisher.h"
#include "score/libTSClient/gptp_ipc_data.h"

#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>

namespace
{
volatile sig_atomic_t g_running = 1;  // NOLINT

void SignalHandler(int /*sig*/) noexcept { g_running = 0; }
}  // namespace

int main(int argc, char* argv[])
{
    std::signal(SIGINT,  SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    const std::string iface = (argc >= 2) ? argv[1] : "emac0";

    std::cout << "[timeslave_standalone] interface = " << iface << '\n';

    // --- Engine options ---
    score::ts::details::GptpEngineOptions opts;
    opts.iface_name           = iface;
    opts.pdelay_interval_ms   = 1000;
    opts.pdelay_warmup_ms     = 2000;
    opts.sync_timeout_ms      = 3300;
    opts.phc_config.enabled             = true;
    opts.phc_config.device              = iface;
    // On Qualcomm EMAC, PTP_SET_TIME only adjusts an offset register; BPF
    // hardware timestamps are unaffected.  A high threshold prevents the step
    // path from blocking the PI frequency controller.
    opts.phc_config.step_threshold_ns   = 10'000'000'000LL;

    score::ts::details::GptpEngine engine{opts};

    if (!engine.Initialize())
    {
        std::cerr << "[timeslave_standalone] ERROR: GptpEngine::Initialize() failed\n"
                  << "  Check: interface name correct? Running as root?\n";
        return 1;
    }
    std::cout << "[timeslave_standalone] GptpEngine initialized\n";

    // --- Shared memory publisher ---
    score::ts::details::GptpIpcPublisher publisher;
    if (!publisher.Init())
    {
        std::cerr << "[timeslave_standalone] ERROR: GptpIpcPublisher::Init() failed\n";
        engine.Deinitialize();
        return 1;
    }
    std::cout << "[timeslave_standalone] Shared memory ready: "
              << score::ts::details::kGptpIpcName << '\n';
    std::cout << "[timeslave_standalone] Running — Ctrl+C to stop\n\n";

    constexpr auto kPublishInterval = std::chrono::milliseconds{50};
    std::uint64_t  publish_count    = 0U;

    while (g_running != 0)
    {
        engine.FinalizeSnapshot();

        score::ts::GptpIpcData data{};
        if (engine.ReadPTPSnapshot(data))
        {
            publisher.Publish(data);
            ++publish_count;

            // Print status every 2 seconds (40 publishes × 50 ms)
            if (publish_count % 40U == 0U)
            {
                const double ptp_sec = static_cast<double>(data.ptp_assumed_time.count()) * 1e-9;
                std::cout << "[" << publish_count << "] "
                          << "ptp=" << ptp_sec << " s"
                          << "  sync="      << std::boolalpha << data.status.is_synchronized
                          << "  timeout="   << data.status.is_timeout
                          << "  correct="   << data.status.is_correct
                          << "  pdelay="    << data.sync_fup_data.pdelay << " ns"
                          << '\n';
            }
        }

        std::this_thread::sleep_for(kPublishInterval);
    }

    std::cout << "\n[timeslave_standalone] Stopping\n";
    engine.Deinitialize();
    publisher.Destroy();
    return 0;
}
