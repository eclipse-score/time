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
 * @brief Vehicle time probe for the end-to-end integration tests.
 *
 * Requests VehicleTime (and the local HighResSteadyTime and SteadyTime for reference) from the
 * time daemon and prints one JSON object per sample on stdout, so that the
 * integration test can parse the result.
 *
 * With --events it also subscribes to the notifications of the vehicle clock (status changes, time
 * synchronization updates, peer delay measurements) and prints one line "EVENT {json}" for each
 * notification. The lines of the samples start with "{", so both can be told apart.
 *
 * Usage: vehicle_time_probe [--samples N] [--interval-ms MS] [--events]
 *   --samples      number of samples to print (default 1; 0 = until SIGINT/SIGTERM)
 *   --interval-ms  delay between samples (default 100)
 *   --events       print the notifications as well
 */

#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"
#include "score/time/steady_time/src/steady_clock.h"
#include "score/time/vehicle_time/src/vehicle_clock.h"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>

namespace
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
volatile std::sig_atomic_t gShutdownRequested{0};

extern "C" void HandleSignal(int /*signal*/) noexcept
{
    gShutdownRequested = 1;
}

// The notifications are delivered by a thread of the vehicle clock, the samples are printed by the main thread.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex gOutputMutex;

bool HasFlag(int argc, const char* argv[], std::string_view name)
{
    for (int i = 1; i < argc; ++i)
    {
        if (name == argv[i])
        {
            return true;
        }
    }
    return false;
}

const char* Bool(const bool value)
{
    return value ? "true" : "false";
}

void PrintStatusEvent(const score::time::VehicleTimeStatus& status)
{
    using Flag = score::time::VehicleTime::StatusFlag;
    const std::lock_guard<std::mutex> lock{gOutputMutex};
    std::cout << "EVENT {\"event\":\"status\",\"synchronized\":" << Bool(status.IsFlagActive(Flag::kSynchronized))
              << ",\"timeout\":" << Bool(status.IsFlagActive(Flag::kTimeOut))
              << ",\"leap_future\":" << Bool(status.IsFlagActive(Flag::kTimeLeapFuture))
              << ",\"leap_past\":" << Bool(status.IsFlagActive(Flag::kTimeLeapPast))
              << ",\"reliable\":" << Bool(status.IsReliable()) << ",\"consistent\":" << Bool(status.IsConsistent())
              << ",\"rate_deviation\":" << status.RateDeviation() << "}" << std::endl;
}

void PrintSyncEvent(const score::time::TimeSlaveSyncData<score::time::VehicleTime>& data)
{
    const std::lock_guard<std::mutex> lock{gOutputMutex};
    std::cout << "EVENT {\"event\":\"sync\",\"sequence_id\":" << data.sequence_id
              << ",\"precise_origin_ns\":" << data.precise_origin_timestamp.time_since_epoch().count()
              << ",\"reference_global_ns\":" << data.reference_global_timestamp.time_since_epoch().count()
              << ",\"reference_local_ns\":" << data.reference_local_timestamp.time_since_epoch().count()
              << ",\"sync_ingress_ns\":" << data.sync_ingress_timestamp.time_since_epoch().count()
              << ",\"correction_field\":" << data.correction_field << ",\"pdelay_ns\":" << data.pdelay.count()
              << ",\"source_clock_identity\":" << data.source_port_identity.clock_identity
              << ",\"source_port\":" << data.source_port_identity.port_number << "}" << std::endl;
}

void PrintPDelayEvent(const score::time::PDelayMeasurementData<score::time::VehicleTime>& data)
{
    const std::lock_guard<std::mutex> lock{gOutputMutex};
    std::cout << "EVENT {\"event\":\"pdelay\",\"sequence_id\":" << data.sequence_id
              << ",\"request_origin_ns\":" << data.request_origin_timestamp.time_since_epoch().count()
              << ",\"request_receipt_ns\":" << data.request_receipt_timestamp.time_since_epoch().count()
              << ",\"response_origin_ns\":" << data.response_origin_timestamp.time_since_epoch().count()
              << ",\"response_receipt_ns\":" << data.response_receipt_timestamp.time_since_epoch().count()
              << ",\"reference_global_ns\":" << data.reference_global_timestamp.time_since_epoch().count()
              << ",\"reference_local_ns\":" << data.reference_local_timestamp.time_since_epoch().count()
              << ",\"pdelay_ns\":" << data.pdelay.count()
              << ",\"request_clock_identity\":" << data.request_port_identity.clock_identity
              << ",\"request_port\":" << data.request_port_identity.port_number
              << ",\"response_clock_identity\":" << data.response_port_identity.clock_identity
              << ",\"response_port\":" << data.response_port_identity.port_number << "}" << std::endl;
}

std::int64_t ParseArg(int argc, const char* argv[], std::string_view name, std::int64_t default_value)
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (name == argv[i])
        {
            return std::strtoll(argv[i + 1], nullptr, 10);
        }
    }
    return default_value;
}

}  // namespace

int main(int argc, const char* argv[])
{
    static_cast<void>(std::signal(SIGINT, HandleSignal));
    static_cast<void>(std::signal(SIGTERM, HandleSignal));

    const std::int64_t samples = ParseArg(argc, argv, "--samples", 1);
    const std::int64_t interval_ms = ParseArg(argc, argv, "--interval-ms", 100);

    auto vehicle_clock = score::time::VehicleClock::GetInstance();
    auto steady_clock = score::time::HighResSteadyClock::GetInstance();
    auto monotonic_clock = score::time::SteadyClock::GetInstance();
    const bool events = HasFlag(argc, argv, "--events");

    // The daemon may not be up yet; Init() is retried with the sampling interval.
    while (!vehicle_clock.Init())
    {
        if (gShutdownRequested != 0)
        {
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{interval_ms});
    }

    if (events)
    {
        vehicle_clock.Subscribe<score::time::VehicleTimeStatus>(&PrintStatusEvent);
        vehicle_clock.Subscribe<score::time::TimeSlaveSyncData<score::time::VehicleTime>>(&PrintSyncEvent);
        vehicle_clock.Subscribe<score::time::PDelayMeasurementData<score::time::VehicleTime>>(&PrintPDelayEvent);
    }

    using Flag = score::time::VehicleTime::StatusFlag;
    for (std::int64_t seq = 0; (samples == 0 || seq < samples) && gShutdownRequested == 0; ++seq)
    {
        const auto vehicle_time = vehicle_clock.Now();
        const auto steady_time = steady_clock.Now();
        const auto monotonic_time = monotonic_clock.Now();
        const auto& status = vehicle_time.Status();

        {
            const std::lock_guard<std::mutex> lock{gOutputMutex};
            std::cout << "{\"seq\":" << seq << ",\"vehicle_time_ns\":" << vehicle_time.TimePointNs().count()
                      << ",\"steady_time_ns\":" << steady_time.TimePointNs().count()
                      << ",\"monotonic_time_ns\":" << monotonic_time.TimePointNs().count()
                      << ",\"synchronized\":" << (status.IsFlagActive(Flag::kSynchronized) ? "true" : "false")
                      << ",\"timeout\":" << (status.IsFlagActive(Flag::kTimeOut) ? "true" : "false")
                      << ",\"leap_future\":" << (status.IsFlagActive(Flag::kTimeLeapFuture) ? "true" : "false")
                      << ",\"leap_past\":" << (status.IsFlagActive(Flag::kTimeLeapPast) ? "true" : "false")
                      << ",\"reliable\":" << (status.IsReliable() ? "true" : "false")
                      << ",\"consistent\":" << (status.IsConsistent() ? "true" : "false")
                      << ",\"rate_deviation\":" << status.RateDeviation() << "}" << std::endl;
        }

        if ((samples == 0 || seq + 1 < samples) && gShutdownRequested == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{interval_ms});
        }
    }

    if (events)
    {
        vehicle_clock.Unsubscribe<score::time::VehicleTimeStatus>();
        vehicle_clock.Unsubscribe<score::time::TimeSlaveSyncData<score::time::VehicleTime>>();
        vehicle_clock.Unsubscribe<score::time::PDelayMeasurementData<score::time::VehicleTime>>();
    }
    return 0;
}
