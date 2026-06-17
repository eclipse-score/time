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
#include "score/time_slave/src/application/time_slave.h"

#include "score/time_slave/src/common/logging_contexts.h"
#include "score/mw/log/logging.h"

#include <thread>

namespace score
{
namespace ts
{

namespace
{

constexpr std::int32_t kInitSuccess = 0;
constexpr std::int32_t kInitFailure = -1;

}  // namespace

TimeSlave::TimeSlave() = default;

std::int32_t TimeSlave::Initialize(const score::mw::lifecycle::ApplicationContext& /*context*/)
{
    engine_ = std::make_unique<details::GptpEngine>(opts_);

    if (!engine_->Initialize())
    {
        score::mw::log::LogError(kTimeSlaveAppContext) << "TimeSlave: GptpEngine initialization failed";
        return kInitFailure;
    }

    if (!publisher_.Open())
    {
        score::mw::log::LogError(kTimeSlaveAppContext) << "TimeSlave: shared memory publisher initialization failed";
        return kInitFailure;
    }

    score::mw::log::LogInfo(kTimeSlaveAppContext) << "TimeSlave initialized";
    return kInitSuccess;
}

std::int32_t TimeSlave::Run(const score::cpp::stop_token& token)
{
    constexpr auto kPublishInterval = std::chrono::milliseconds{50};

    score::mw::log::LogInfo(kTimeSlaveAppContext) << "TimeSlave running";

    while (!token.stop_requested())
    {
        engine_->FinalizeSnapshot();
        score::ts::GptpIpcData data{};
        if (engine_->ReadPTPSnapshot(data))
        {
            publisher_.Publish(data);
        }

        std::this_thread::sleep_for(kPublishInterval);
    }

    engine_->Deinitialize();
    publisher_.Close();

    score::mw::log::LogInfo(kTimeSlaveAppContext) << "TimeSlave stopped";
    return kInitSuccess;
}

}  // namespace ts
}  // namespace score
