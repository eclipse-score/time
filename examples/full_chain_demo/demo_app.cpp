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

// Usage: ./demo_app [iterations]   (default: runs forever until Ctrl-C)

#include "score/time/src/synchronized_vehicle_time/factory.h"
#include "score/time/src/synchronized_vehicle_time/details/factory_impl.h"
#include "score/time/src/synchronized_vehicle_time/synchronized_vehicle_time.h"

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

    score::time::SynchronizedVehicleTime::FactoryImpl factory;
    auto svt = factory.ObtainSynchronizedSlaveTimebase();

    std::printf("[demo_app] waiting for TimeDaemon SVT channel...\n");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while (!svt->IsAvailable() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
    }
    if (!svt->IsAvailable())
    {
        std::fprintf(stderr, "[demo_app] ERROR: TimeDaemon SVT channel not available after 10 s\n");
        return 1;
    }
    std::printf("[demo_app] SVT channel available, reading time\n");

    int iter = 0;
    while (g_running && (iterations == 0 || iter < iterations))
    {
        auto ts = svt->Now();
        const auto ptp_ns  = ts.getTimepoint().time_since_epoch().count();
        const auto& status = ts.getTimepointStatus();

        const char* flag = "UNKNOWN";
        if (status.IsFlagActive(score::time::SynchronizedVehicleTime::StatusFlag::kSynchronized))
            flag = "SYNCHRONIZED";
        else if (status.IsFlagActive(score::time::SynchronizedVehicleTime::StatusFlag::kTimeOut))
            flag = "TIMEOUT";
        else if (status.IsFlagActive(score::time::SynchronizedVehicleTime::StatusFlag::kTimeLeapFuture))
            flag = "LEAP_FUTURE";
        else if (status.IsFlagActive(score::time::SynchronizedVehicleTime::StatusFlag::kTimeLeapPast))
            flag = "LEAP_PAST";

        std::printf("[demo_app] ptp_time=%.6f s  rate_dev=%.2e  status=%s\n",
                    static_cast<double>(ptp_ns) * 1e-9,
                    svt->GetRateDeviation(),
                    flag);

        ++iter;
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }

    std::printf("[demo_app] stopped after %d reads\n", iter);
    return 0;
}
