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
 * @file
 * @brief Example: periodic VehicleTime + HighResSteadyTime report printer.
 *
 * Uses VehicleTimeHandler to obtain a combined snapshot of the PTP-synchronized
 * vehicle time and the local monotonic HIRS time, then prints both to stdout
 * once per second until interrupted by SIGINT or SIGTERM.
 *
 * The handler class can be unit-tested in isolation — see time_handler_test.cpp.
 */

#include "examples/time/vehicle_time/src/vehicle_time_handler.h"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <thread>

namespace
{

/** @brief Flag set by the signal handler to request a clean shutdown. */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
volatile std::sig_atomic_t gShutdownRequested{0};

/** @brief Signal handler for SIGINT / SIGTERM. */
extern "C" void HandleSignal(int /*signal*/) noexcept
{
    gShutdownRequested = 1;
}

/**
 * @brief Prints a single TimeReport to stdout.
 *
 * @param report  The combined time report to format.
 * @param seq     Monotonic sequence number of this print.
 */
void PrintReport(const examples::time::vehicle_time::TimeReport& report, std::uint64_t seq) noexcept
{
    const auto v_sec  = report.vehicle_time_ns / 1'000'000'000LL;
    const auto v_ns   = report.vehicle_time_ns % 1'000'000'000LL;
    const auto h_sec  = report.high_res_steady_time_ns    / 1'000'000'000LL;
    const auto h_ns   = report.high_res_steady_time_ns    % 1'000'000'000LL;

    std::cout << "[" << seq << "]"
              << "  vehicle=" << v_sec << "." << v_ns << " s"
              << "  hirs="    << h_sec << "." << h_ns << " s"
              << "  is_reliable="   << (report.is_reliable   ? "yes" : "no")
              << "  is_consistent=" << (report.is_consistent ? "yes" : "no")
              << "  rate_deviation=" << report.rate_deviation
              << "\n";
}

}  // namespace

int main()
{
    static_cast<void>(std::signal(SIGINT,  HandleSignal));
    static_cast<void>(std::signal(SIGTERM, HandleSignal));

    examples::time::vehicle_time::VehicleTimeHandler handler;

    if (!handler.Init())
    {
        std::cerr << "Failed to initialise VehicleTime backend. Exiting.\n";
        return 1;
    }

    std::cout << "VehicleTime + HighResSteadyTime printer started. Press Ctrl+C to stop.\n";

    std::uint64_t seq{0U};

    while (gShutdownRequested == 0)
    {
        const auto report = handler.GetCurrentTime();
        PrintReport(report, seq);
        ++seq;

        std::this_thread::sleep_for(std::chrono::seconds{1});
    }

    std::cout << "Shutdown requested. Exiting.\n";
    return 0;
}
