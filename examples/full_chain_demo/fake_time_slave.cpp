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

// Usage: ./fake_time_slave [iterations]   (default: runs forever until Ctrl-C)

#include "score/ts_client/src/gptp_ipc_publisher.h"
#include "score/ts_client/src/gptp_ipc_data.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <thread>

static volatile bool g_running = true;
static void on_signal(int) { g_running = false; }

int main(int argc, char* argv[])
{
    const int iterations = (argc > 1) ? std::atoi(argv[1]) : 0;  // 0 = forever

    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    score::ts::details::GptpIpcPublisher pub;
    if (!pub.Init())
    {
        std::fprintf(stderr, "[fake_time_slave] ERROR: failed to create shared memory\n");
        return 1;
    }
    std::printf("[fake_time_slave] shared memory created, publishing at 50 ms\n");

    const auto t0 = std::chrono::steady_clock::now();
    int seq = 0;

    while (g_running && (iterations == 0 || seq < iterations))
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - t0);

        score::ts::GptpIpcData data{};
        data.ptp_assumed_time             = elapsed + std::chrono::nanoseconds{1'600'000'000'000'000'000LL};
        data.local_time                   = elapsed;
        data.rate_deviation               = 5e-7;
        data.status.is_synchronized       = true;
        data.status.is_correct            = true;
        data.status.is_timeout            = false;
        data.status.is_time_jump_future   = false;
        data.status.is_time_jump_past     = false;
        data.sync_fup_data.sequence_id             = static_cast<std::uint16_t>(seq & 0xFFFF);
        data.sync_fup_data.pdelay                  = 1500U;
        data.sync_fup_data.port_number             = 1;
        data.sync_fup_data.sync_ingress_timestamp  = static_cast<std::uint64_t>(elapsed.count());
        data.sync_fup_data.precise_origin_timestamp = static_cast<std::uint64_t>(elapsed.count());

        pub.Publish(data);
        std::printf("[fake_time_slave] seq=%-5d  ptp=%.3f s\n",
                    seq,
                    static_cast<double>(data.ptp_assumed_time.count()) * 1e-9);

        ++seq;
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }

    std::printf("[fake_time_slave] stopped after %d frames\n", seq);
    return 0;
}
