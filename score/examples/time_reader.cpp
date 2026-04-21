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
 * @brief Demo: libTSClient GptpIpcReceiver consumer.
 *
 * Opens the shared memory segment written by TimeSlave (or time_publisher)
 * and continuously reads + displays the GptpIpcData using the seqlock protocol.
 *
 * Usage:
 *   bazel run --config time-x86_64-linux //score/examples:time_reader
 *
 * Requires time_publisher (or real TimeSlave) to be running first.
 */

#include "score/libTSClient/gptp_ipc_receiver.h"
#include "score/libTSClient/gptp_ipc_data.h"
#include "score/libTSClient/gptp_ipc_channel.h"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace
{
volatile sig_atomic_t g_running = 1;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void SignalHandler(int /*sig*/) noexcept
{
    g_running = 0;
}

/// Return a formatted wall-clock timestamp string "HH:MM:SS.mmm".
std::string Timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_buf);
    std::ostringstream oss;
    oss << buf << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

void PrintData(const score::ts::GptpIpcData& d)
{
    const double ptp_sec   = static_cast<double>(d.ptp_assumed_time.count()) * 1e-9;
    const double local_sec = static_cast<double>(d.local_time.count()) * 1e-9;

    std::cout << "  ptp_assumed_time  : " << std::fixed << std::setprecision(9) << ptp_sec << " s\n";
    std::cout << "  local_time        : " << std::fixed << std::setprecision(9) << local_sec << " s\n";
    std::cout << "  rate_deviation    : " << std::scientific << std::setprecision(3) << d.rate_deviation << '\n';

    // Status flags
    std::cout << "  status            :"
              << (d.status.is_synchronized    ? " [SYNC]"         : " [NO-SYNC]")
              << (d.status.is_timeout         ? " [TIMEOUT]"      : "")
              << (d.status.is_time_jump_future ? " [JUMP-FUTURE]"  : "")
              << (d.status.is_time_jump_past   ? " [JUMP-PAST]"    : "")
              << (d.status.is_correct          ? " [CORRECT]"      : " [INCORRECT]")
              << '\n';

    // Sync+FollowUp data
    const auto& s = d.sync_fup_data;
    std::cout << "  sync_fup.seq_id   : " << s.sequence_id << '\n';
    std::cout << "  sync_fup.pdelay   : " << s.pdelay << " ns\n";
    std::cout << "  sync_fup.port     : " << s.port_number << '\n';
    std::cout << "  sync_fup.clk_id   : 0x"
              << std::hex << std::setw(16) << std::setfill('0') << s.clock_identity
              << std::dec << '\n';

    // Peer delay data
    const auto& p = d.pdelay_data;
    std::cout << "  pdelay.pdelay     : " << p.pdelay << " ns\n";
    std::cout << "  pdelay.req_port   : " << p.req_port_number << '\n';
    std::cout << "  pdelay.resp_port  : " << p.resp_port_number << '\n';
}

}  // namespace

int main()
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    score::ts::details::GptpIpcReceiver receiver;

    std::cout << "[time_reader] Waiting for shared memory '"
              << score::ts::details::kGptpIpcName << "' ...\n";

    // Retry Init until the publisher creates the shared memory segment
    while (g_running != 0)
    {
        if (receiver.Init())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
    }

    if (g_running == 0)
    {
        std::cout << "\n[time_reader] Interrupted before connecting.\n";
        return 0;
    }

    std::cout << "[time_reader] Connected. Reading every 100 ms. Press Ctrl+C to stop.\n\n";

    std::uint64_t read_count    = 0U;
    std::uint64_t nullopt_count = 0U;

    while (g_running != 0)
    {
        const auto result = receiver.Receive();

        if (result.has_value())
        {
            ++read_count;
            // Print full detail every 10 reads (~1 s), summary otherwise
            if (read_count % 10U == 1U)
            {
                std::cout << "--- [" << Timestamp() << "]  read #" << read_count
                          << "  (contention_misses=" << nullopt_count << ") ---\n";
                PrintData(*result);
                std::cout << '\n';
            }
        }
        else
        {
            ++nullopt_count;
            // nullopt means seqlock contention; just retry next cycle
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    std::cout << "\n[time_reader] Stopping. total_reads=" << read_count
              << "  contention_misses=" << nullopt_count << '\n';
    receiver.Close();
    return 0;
}
