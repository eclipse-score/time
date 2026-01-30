/********************************************************************************************************
# Copyright ©, 2026, ECARX. All rights reserved. This statement is the current and authoritative version.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: chenhao.gao chenhao.gao@ecarxgroup.com
*********************************************************************************************************/
#include <atomic>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "time_sync_engine.hpp"
#include "config_loader.hpp"

static std::atomic<bool> g_exit{false};

static void signal_handler(int signum)
{
    if (signum == SIGTERM || signum == SIGINT)
        g_exit.store(true, std::memory_order_release);
}

static void init_signals()
{
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);
}

int main(int argc, char **argv)
{
    init_signals();

    tsyncd::EngineOptions opt;

    std::string config_path = "./tsyncd.conf";
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc)
        {
            config_path = argv[++i];
        }
    }

    tsyncd::LoadEngineOptionsFromFile(config_path, opt);

    tsyncd::TimeSyncEngine engine(opt);
    if (!engine.Start())
    {
        std::fprintf(stderr, "tsyncd: failed to start\n");
        return 1;
    }

    while (!g_exit.load(std::memory_order_acquire))
    {
        ::sleep(1);
    }

    engine.Stop();
    std::fprintf(stderr, "tsyncd: exit\n");
    return 0;
}
