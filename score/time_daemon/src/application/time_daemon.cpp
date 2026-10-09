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
#include "score/time_daemon/src/application/time_daemon.h"
#include "score/time_daemon/src/common/logging_contexts.h"

#include "score/concurrency/interruptible_wait.h"
#include "score/mw/log/logging.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <utility>

namespace score
{
namespace td
{

TimeDaemon::TimeDaemon(std::vector<std::unique_ptr<TimebaseHandler>> timebase_handlers)
    : score::mw::lifecycle::Application(), timebase_handlers_{std::move(timebase_handlers)}
{
}

std::int32_t TimeDaemon::Initialize(const score::mw::lifecycle::ApplicationContext&)
{
    score::mw::log::LogInfo(kAppContext) << "TimeDaemon initializing...";

    if (timebase_handlers_.empty() ||
        std::any_of(timebase_handlers_.cbegin(), timebase_handlers_.cend(), [](const auto& handler) {
            return handler == nullptr;
        }))
    {
        score::mw::log::LogError(kAppContext) << "TimeDaemon requires at least one handler and no null handlers";
        return EXIT_FAILURE;
    }

    for (const auto& handler : timebase_handlers_)
    {
        handler->Initialize();
    }

    score::mw::log::LogInfo(kAppContext) << "TimeDaemon initialized";
    return EXIT_SUCCESS;
}

std::int32_t TimeDaemon::Run(const score::cpp::stop_token& token)
{
    score::mw::log::LogInfo(kAppContext) << "Run() started";

    while (!token.stop_requested())
    {
        for (const auto& handler : timebase_handlers_)
        {
            handler->RunOnce(token);
        }
        score::concurrency::wait_for(token, std::chrono::milliseconds(100));
    }

    for (auto handler = timebase_handlers_.rbegin(); handler != timebase_handlers_.rend(); ++handler)
    {
        (*handler)->Stop();
    }

    score::mw::log::LogInfo(kAppContext) << "Run() finished";
    return EXIT_SUCCESS;
}

}  // namespace td
}  // namespace score
