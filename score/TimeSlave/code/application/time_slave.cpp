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
#include "score/TimeSlave/code/application/time_slave.h"

#include "score/TimeDaemon/code/common/logging_contexts.h"
#include "score/mw/log/logging.h"
#include "score/time/HighPrecisionLocalSteadyClock/details/factory_impl.h"

#include <thread>

namespace score
{
namespace ts
{

TimeSlave::TimeSlave() = default;

std::int32_t TimeSlave::Initialize(const score::mw::lifecycle::ApplicationContext& /*context*/)
{
    // Create the high-precision local clock for the gPTP engine
    score::time::HighPrecisionLocalSteadyClock::FactoryImpl clock_factory{};
    auto clock = clock_factory.CreateHighPrecisionLocalSteadyClock();

    engine_ = std::make_unique<details::GptpEngine>(opts_, std::move(clock));

    if (!engine_->Initialize())
    {
        score::mw::log::LogError(kGPtpMachineContext) << "TimeSlave: GptpEngine initialization failed";
        return -1;
    }

    if (!publisher_.Init())
    {
        score::mw::log::LogError(kGPtpMachineContext) << "TimeSlave: shared memory publisher initialization failed";
        return -1;
    }

    score::mw::log::LogInfo(kGPtpMachineContext) << "TimeSlave initialized";
    return 0;
}

std::int32_t TimeSlave::Run(const score::cpp::stop_token& token)
{
    constexpr auto kPublishInterval = std::chrono::milliseconds{50};

    score::mw::log::LogInfo(kGPtpMachineContext) << "TimeSlave running";

    while (!token.stop_requested())
    {
        PtpTimeInfo info{};
        if (engine_->ReadPTPSnapshot(info))
        {
            publisher_.Publish(info);
        }

        std::this_thread::sleep_for(kPublishInterval);
    }

    engine_->Deinitialize();
    publisher_.Destroy();

    score::mw::log::LogInfo(kGPtpMachineContext) << "TimeSlave stopped";
    return 0;
}

}  // namespace ts
}  // namespace score
