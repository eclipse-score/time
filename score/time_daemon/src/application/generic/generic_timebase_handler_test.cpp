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
#include "score/time_daemon/src/application/generic/generic_timebase_handler.h"

#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/data_flow/producer.h"
#include "score/time_daemon/src/common/machines/proactive_machine.h"
#include "score/time_daemon/src/common/machines/reactive_machine.h"
#include "score/time_daemon/src/msg_broker/topic.h"

#include <gtest/gtest.h>
#include <score/stop_token.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace score
{
namespace td
{
namespace
{

constexpr int kPublishedValue{42};

/**
 * @brief Proactive machine that publishes kPublishedValue when it is started.
 *
 * Init() only reads state that is set at construction, because it runs on the JobRunner thread.
 * Start() and Stop() run on the test thread.
 */
class FakeSource final : public ProactiveMachine, public Producer<int>
{
  public:
    FakeSource(const std::string& name, std::vector<std::string>& events, bool init_result)
        : ProactiveMachine{name}, events_{events}, init_result_{init_result}, started_{false}, callback_{}
    {
    }

    bool Init() override
    {
        return init_result_;
    }

    void Start() noexcept override
    {
        started_ = true;
        events_.push_back("start:" + GetName());
        Publish(kPublishedValue);
    }

    void Stop() noexcept override
    {
        events_.push_back("stop:" + GetName());
    }

    void SetPublishCallback(std::function<void(const int&)> callback) override
    {
        callback_ = std::move(callback);
    }

    bool IsStarted() const noexcept
    {
        return started_;
    }

  private:
    void Publish(const int& data) override
    {
        if (callback_)
        {
            callback_(data);
        }
    }

    std::vector<std::string>& events_;
    bool init_result_;
    bool started_;
    std::function<void(const int&)> callback_;
};

/**
 * @brief Reactive machine that records the messages it receives.
 */
class FakeSink final : public ReactiveMachine, public Consumer<int>
{
  public:
    explicit FakeSink(const std::string& name) : ReactiveMachine{name}, received_{} {}

    bool Init() override
    {
        return true;
    }

    void OnMessage(int data) override
    {
        received_.push_back(data);
    }

    const std::vector<int>& Received() const noexcept
    {
        return received_;
    }

  private:
    std::vector<int> received_;
};

/**
 * @brief Test fixture for GenericTimebaseHandler tests.
 */
class GenericTimebaseHandlerTest : public ::testing::Test
{
  protected:
    /**
     * @brief Calls RunOnce() on the test thread until the predicate holds or the timeout expires.
     */
    template <typename Predicate>
    bool RunUntil(GenericTimebaseHandler<int>& handler, Predicate predicate, std::chrono::seconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!predicate())
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                return false;
            }
            handler.RunOnce(stop_token_);
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        return true;
    }

    score::cpp::stop_source stop_source_{};
    /// The JobRunner refers to the token until the init jobs finish, so the token is kept in the fixture.
    score::cpp::stop_token stop_token_{stop_source_.get_token()};
};

/**
 * @brief Test that proactive machines start in the order they were added and stop in reverse order.
 */
TEST_F(GenericTimebaseHandlerTest, StartsProactiveMachinesInAddOrderAndStopsInReverse)
{
    std::vector<std::string> events;
    GenericTimebaseHandler<int> handler{"start_order"};
    handler.Add(std::make_shared<FakeSource>("first", events, true));
    auto second = handler.Add(std::make_shared<FakeSource>("second", events, true));
    handler.Initialize();

    ASSERT_TRUE(RunUntil(
        handler,
        [&second] {
            return second->IsStarted();
        },
        std::chrono::seconds{5}));
    EXPECT_EQ(events, (std::vector<std::string>{"start:first", "start:second"}));

    handler.Stop();
    EXPECT_EQ(events, (std::vector<std::string>{"start:first", "start:second", "stop:second", "stop:first"}));
}

/**
 * @brief Test that data published by a producer reaches the consumers subscribed to the same topic.
 */
TEST_F(GenericTimebaseHandlerTest, DeliversDataOfPublisherToSubscribersOfTheSameTopic)
{
    std::vector<std::string> events;
    GenericTimebaseHandler<int> handler{"delivery"};
    const Topic topic{"values"};
    auto source = handler.Add(std::make_shared<FakeSource>("source", events, true));
    auto sink = handler.Add(std::make_shared<FakeSink>("sink"));
    handler.Publish(source, topic);
    handler.Subscribe(topic, sink);
    handler.Initialize();

    ASSERT_TRUE(RunUntil(
        handler,
        [&source] {
            return source->IsStarted();
        },
        std::chrono::seconds{5}));
    EXPECT_EQ(sink->Received(), std::vector<int>{kPublishedValue});
}

/**
 * @brief Test that a proactive machine whose initialization times out is never started.
 */
TEST_F(GenericTimebaseHandlerTest, DoesNotStartProactiveMachinesWhenInitializationTimesOut)
{
    std::vector<std::string> events;
    GenericTimebaseHandler<int> handler{"init_timeout"};
    auto source = handler.Add(std::make_shared<FakeSource>("source", events, false), std::chrono::seconds{1});
    handler.Initialize();

    EXPECT_FALSE(RunUntil(
        handler,
        [&source] {
            return source->IsStarted();
        },
        std::chrono::seconds{2}));
    EXPECT_TRUE(events.empty());
}

}  // namespace
}  // namespace td
}  // namespace score
