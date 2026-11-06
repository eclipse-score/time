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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <score/stop_token.hpp>

#include <array>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

namespace score
{
namespace td
{
namespace
{

class TimebaseHandlerMock : public TimebaseHandler
{
  public:
    explicit TimebaseHandlerMock(bool* destroyed = nullptr) : destroyed_{destroyed} {}

    ~TimebaseHandlerMock() noexcept override
    {
        if (destroyed_ != nullptr)
        {
            *destroyed_ = true;
        }
    }

    MOCK_METHOD(void, Initialize, (), (noexcept, override));
    MOCK_METHOD(void, Start, (const score::cpp::stop_token&), (noexcept, override));
    MOCK_METHOD(void, RunOnce, (), (noexcept, override));
    MOCK_METHOD(void, Stop, (), (noexcept, override));

  private:
    bool* destroyed_;
};

}  // namespace

std::vector<std::unique_ptr<TimebaseHandler>> CreateTimebaseHandlers()
{
    auto handler = std::make_unique<::testing::StrictMock<TimebaseHandlerMock>>();
    EXPECT_CALL(*handler, Initialize());

    std::vector<std::unique_ptr<TimebaseHandler>> handlers;
    handlers.push_back(std::move(handler));
    return handlers;
}

namespace
{

class TimeDaemonTest : public ::testing::Test
{
  protected:
    TimebaseHandlerMock& AddHandler(bool* destroyed = nullptr)
    {
        auto handler = std::make_unique<::testing::StrictMock<TimebaseHandlerMock>>(destroyed);
        auto& result = *handler;
        handlers_.push_back(std::move(handler));
        return result;
    }

    std::array<const char*, 1> arguments_{"time_daemon_test"};
    score::mw::lifecycle::ApplicationContext context_{1, arguments_.data()};
    score::cpp::stop_source stop_source_{};
    score::cpp::stop_token stop_token_{stop_source_.get_token()};
    std::vector<std::unique_ptr<TimebaseHandler>> handlers_;
};

TEST_F(TimeDaemonTest, InitializesHandlersFromLinkedFactory)
{
    TimeDaemon daemon{};

    EXPECT_EQ(daemon.Initialize(context_), EXIT_SUCCESS);
}

TEST_F(TimeDaemonTest, RunsOneInjectedHandler)
{
    auto& handler = AddHandler();
    TimeDaemon daemon{std::move(handlers_)};
    const ::testing::InSequence sequence;
    EXPECT_CALL(handler, Initialize());
    EXPECT_CALL(handler, Start(::testing::Ref(stop_token_)));
    EXPECT_CALL(handler, RunOnce()).WillOnce([this]() {
        stop_source_.request_stop();
    });
    EXPECT_CALL(handler, Stop());

    ASSERT_EQ(daemon.Initialize(context_), EXIT_SUCCESS);
    EXPECT_EQ(daemon.Run(stop_token_), EXIT_SUCCESS);
}

TEST_F(TimeDaemonTest, PollsEveryHandlerInOrderAndStopsInReverseOrder)
{
    auto& first = AddHandler();
    auto& second = AddHandler();
    TimeDaemon daemon{std::move(handlers_)};
    const ::testing::InSequence sequence;
    EXPECT_CALL(first, Initialize());
    EXPECT_CALL(second, Initialize());
    EXPECT_CALL(first, Start(::testing::Ref(stop_token_)));
    EXPECT_CALL(second, Start(::testing::Ref(stop_token_)));
    EXPECT_CALL(first, RunOnce());
    EXPECT_CALL(second, RunOnce());
    EXPECT_CALL(first, RunOnce());
    EXPECT_CALL(second, RunOnce()).WillOnce([this]() {
        stop_source_.request_stop();
    });
    EXPECT_CALL(second, Stop());
    EXPECT_CALL(first, Stop());

    ASSERT_EQ(daemon.Initialize(context_), EXIT_SUCCESS);
    EXPECT_EQ(daemon.Run(stop_token_), EXIT_SUCCESS);
}

TEST_F(TimeDaemonTest, CompletesThePollingPassWhenAHandlerRequestsStop)
{
    auto& first = AddHandler();
    auto& second = AddHandler();
    TimeDaemon daemon{std::move(handlers_)};
    const ::testing::InSequence sequence;
    EXPECT_CALL(first, Initialize());
    EXPECT_CALL(second, Initialize());
    EXPECT_CALL(first, Start(::testing::Ref(stop_token_)));
    EXPECT_CALL(second, Start(::testing::Ref(stop_token_)));
    EXPECT_CALL(first, RunOnce()).WillOnce([this]() {
        stop_source_.request_stop();
    });
    EXPECT_CALL(second, RunOnce()).WillOnce([this]() {
        EXPECT_TRUE(stop_token_.stop_requested());
    });
    EXPECT_CALL(second, Stop());
    EXPECT_CALL(first, Stop());

    ASSERT_EQ(daemon.Initialize(context_), EXIT_SUCCESS);
    EXPECT_EQ(daemon.Run(stop_token_), EXIT_SUCCESS);
}

TEST_F(TimeDaemonTest, StopsInitializedHandlersWithoutPollingWhenAlreadyCancelled)
{
    auto& first = AddHandler();
    auto& second = AddHandler();
    TimeDaemon daemon{std::move(handlers_)};
    const ::testing::InSequence sequence;
    EXPECT_CALL(first, Initialize());
    EXPECT_CALL(second, Initialize());
    EXPECT_CALL(first, Start(::testing::Ref(stop_token_)));
    EXPECT_CALL(second, Start(::testing::Ref(stop_token_)));
    EXPECT_CALL(second, Stop());
    EXPECT_CALL(first, Stop());

    ASSERT_EQ(daemon.Initialize(context_), EXIT_SUCCESS);
    stop_source_.request_stop();
    EXPECT_EQ(daemon.Run(stop_token_), EXIT_SUCCESS);
}

TEST_F(TimeDaemonTest, RejectsEmptyComposition)
{
    TimeDaemon daemon{std::move(handlers_)};

    EXPECT_EQ(daemon.Initialize(context_), EXIT_FAILURE);
}

TEST_F(TimeDaemonTest, RejectsNullEntriesBeforeInitializingAnyHandler)
{
    auto& handler = AddHandler();
    handlers_.emplace_back();
    TimeDaemon daemon{std::move(handlers_)};
    EXPECT_CALL(handler, Initialize()).Times(0);

    EXPECT_EQ(daemon.Initialize(context_), EXIT_FAILURE);
}

TEST_F(TimeDaemonTest, OwnsInjectedHandlersUntilDestruction)
{
    bool destroyed{false};
    AddHandler(&destroyed);
    {
        TimeDaemon daemon{std::move(handlers_)};
        EXPECT_FALSE(destroyed);
    }
    EXPECT_TRUE(destroyed);
}

}  // namespace
}  // namespace td
}  // namespace score
