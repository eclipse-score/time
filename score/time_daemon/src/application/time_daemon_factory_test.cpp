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
#include "score/time_daemon/src/application/timebase_handler_factory.h"

#include <gtest/gtest.h>
#include <score/stop_token.hpp>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace score
{
namespace td
{
namespace
{

struct FactoryState
{
    std::size_t calls{0U};
    std::vector<std::string> events;
    score::cpp::stop_source stop_source{};
};

FactoryState factory_state;

class FactoryHandler final : public TimebaseHandler
{
  public:
    explicit FactoryHandler(std::string name, bool request_stop) : name_{std::move(name)}, request_stop_{request_stop}
    {
    }

    void Initialize() noexcept override
    {
        factory_state.events.push_back("initialize:" + name_);
    }

    void RunOnce(const score::cpp::stop_token&) noexcept override
    {
        factory_state.events.push_back("run:" + name_);
        if (request_stop_)
        {
            factory_state.stop_source.request_stop();
        }
    }

    void Stop() noexcept override
    {
        factory_state.events.push_back("stop:" + name_);
    }

  private:
    std::string name_;
    bool request_stop_;
};

TEST(TimeDaemonFactoryTest, CreatesTheLinkedCompositionOnceAndRunsEveryHandler)
{
    factory_state = FactoryState{};
    const std::array<const char*, 1> arguments{"time_daemon_factory_test"};
    const score::mw::lifecycle::ApplicationContext context{1, arguments.data()};
    const auto token = factory_state.stop_source.get_token();
    TimeDaemon daemon{};

    EXPECT_EQ(factory_state.calls, 1U);
    ASSERT_EQ(daemon.Initialize(context), EXIT_SUCCESS);
    EXPECT_EQ(daemon.Run(token), EXIT_SUCCESS);
    EXPECT_EQ(factory_state.calls, 1U);
    EXPECT_EQ(factory_state.events,
              (std::vector<std::string>{
                  "initialize:first", "initialize:second", "run:first", "run:second", "stop:second", "stop:first"}));
}

}  // namespace

std::vector<std::unique_ptr<TimebaseHandler>> CreateTimebaseHandlers()
{
    ++factory_state.calls;
    std::vector<std::unique_ptr<TimebaseHandler>> handlers;
    handlers.push_back(std::make_unique<FactoryHandler>("first", false));
    handlers.push_back(std::make_unique<FactoryHandler>("second", true));
    return handlers;
}

}  // namespace td
}  // namespace score
