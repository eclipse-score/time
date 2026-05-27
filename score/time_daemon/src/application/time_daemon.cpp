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
#include "score/time_daemon/src/application/svt/factory.h"
#include "score/time_daemon/src/common/logging_contexts.h"

#include "score/mw/log/logging.h"
#include "score/concurrency/interruptible_wait.h"

namespace score
{
namespace td
{

TimeDaemon::TimeDaemon() : score::mw::lifecycle::Application()
{
    svt_timebase_handler_ = CreateSvtTimebase();
}

std::int32_t TimeDaemon::Initialize(const score::mw::lifecycle::ApplicationContext&)
{
    score::mw::log::LogInfo(kAppContext) << "TimeDaemon initializing...";

    svt_timebase_handler_->Initialize();

    score::mw::log::LogInfo(kAppContext) << "TimeDaemon initialized";
    return EXIT_SUCCESS;
}

std::int32_t TimeDaemon::Run(const score::cpp::stop_token& token)
{
    score::mw::log::LogInfo(kAppContext) << "Run() started";

    while (!token.stop_requested())
    {
        svt_timebase_handler_->RunOnce(token);
        score::concurrency::wait_for(token, std::chrono::milliseconds(100));
    }

    svt_timebase_handler_->Stop();

    score::mw::log::LogInfo(kAppContext) << "Run() finished";
    return EXIT_SUCCESS;
}

}  // namespace td
}  // namespace score
