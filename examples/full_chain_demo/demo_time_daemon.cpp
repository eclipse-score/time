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

// NOTE: The BUILD target for this binary lives in
//   //score/time_daemon/src/application:demo_time_daemon
// because svt_handler_shm is only visible to that package.
// See score/time_daemon/src/application/svt/svt_handler_shm.h for the
// implementation that wires GPTPShmMachine into the SVT pipeline.

#include "score/time_daemon/src/application/svt/factory_shm.h"
#include "score/concurrency/interruptible_wait.h"

#include <score/stop_token.hpp>
#include <csignal>
#include <cstdio>
#include <chrono>

static score::cpp::stop_source* g_stop = nullptr;

static void on_signal(int)
{
    if (g_stop != nullptr)
    {
        g_stop->request_stop();
    }
}

int main()
{
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    score::cpp::stop_source stop_source;
    g_stop = &stop_source;

    auto handler = score::td::CreateSvtTimebaseShm();
    handler->Initialize();

    std::printf("[demo_time_daemon] started — reading gPTP SHM, publishing SVT\n");

    const score::cpp::stop_token token = stop_source.get_token();
    while (!token.stop_requested())
    {
        handler->RunOnce(token);
        score::concurrency::wait_for(token, std::chrono::milliseconds{100});
    }

    handler->Stop();
    std::printf("[demo_time_daemon] stopped\n");
    return 0;
}
