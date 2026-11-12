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
#include "score/time_daemon/src/application/timebase_handlers/timebase_handler_factory.h"
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

TimeDaemon::TimeDaemon() : TimeDaemon{CreateTimebaseHandlers()} {}

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

    InitializeAllTimebases();

    score::mw::log::LogInfo(kAppContext) << "TimeDaemon initialized";
    return EXIT_SUCCESS;
}

std::int32_t TimeDaemon::Run(const score::cpp::stop_token& token)
{
    score::mw::log::LogInfo(kAppContext) << "Run() started";

    StartAllTimebases(token);

    bool failed{false};
    while (!token.stop_requested() && !failed)
    {
        IterateAllTimebases();
        failed = AnyTimebaseFailed();
        if (!failed)
        {
            score::concurrency::wait_for(token, std::chrono::milliseconds(100));
        }
    }

    StopAllTimebases();

    if (failed)
    {
        score::mw::log::LogError(kAppContext) << "Run() finished: timebase initialization failed";
        return EXIT_FAILURE;
    }
    score::mw::log::LogInfo(kAppContext) << "Run() finished";
    return EXIT_SUCCESS;
}

void TimeDaemon::InitializeAllTimebases()
{
    for (const auto& handler : timebase_handlers_)
    {
        handler->Initialize();
    }
}

void TimeDaemon::StartAllTimebases(const score::cpp::stop_token& token)
{
    for (const auto& handler : timebase_handlers_)
    {
        handler->Start(token);
    }
}

void TimeDaemon::IterateAllTimebases()
{
    for (const auto& handler : timebase_handlers_)
    {
        handler->RunOnce();
    }
}

bool TimeDaemon::AnyTimebaseFailed() const
{
    return std::any_of(timebase_handlers_.cbegin(), timebase_handlers_.cend(), [](const auto& handler) {
        return handler->IsFailed();
    });
}

void TimeDaemon::StopAllTimebases()
{
    for (auto handler = timebase_handlers_.rbegin(); handler != timebase_handlers_.rend(); ++handler)
    {
        (*handler)->Stop();
    }
}

}  // namespace td
}  // namespace score
