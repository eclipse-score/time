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
 * @brief Example: periodic HighResSteadyTime report printer.
 *
 * Uses HighResSteadyTimeHandler to obtain the current high-precision local steady time
 * and prints it to stdout once per second until interrupted by SIGINT or SIGTERM.
 *
 * The handler class can be unit-tested in isolation — see time_handler_test.cpp.
 */

#include "examples/time/high_res_steady_time/src/high_res_steady_time_handler.h"

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
 * @param report  The time report to format.
 * @param seq     Monotonic sequence number of this print.
 */
void PrintReport(const examples::time::high_res_steady_time::TimeReport& report, std::uint64_t seq) noexcept
{
    const auto seconds     = report.time_ns / 1'000'000'000LL;
    const auto nanoseconds = report.time_ns % 1'000'000'000LL;

    std::cout << "[" << seq << "]"
              << "  time=" << seconds << "." << nanoseconds << " s\n";
}

}  // namespace

int main()
{
    static_cast<void>(std::signal(SIGINT,  HandleSignal));
    static_cast<void>(std::signal(SIGTERM, HandleSignal));

    examples::time::high_res_steady_time::HighResSteadyTimeHandler handler;

    std::cout << "HighResSteadyTime printer started. Press Ctrl+C to stop.\n";

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
