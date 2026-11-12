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
#include "score/time_daemon/src/application/timebase_handlers/generic/generic_timebase_handler.h"

#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/data_flow/producer.h"
#include "score/time_daemon/src/common/machines/periodic_machine.h"
#include "score/time_daemon/src/common/machines/proactive_machine.h"
#include "score/time_daemon/src/common/machines/reactive_machine.h"
#include "score/time_daemon/src/msg_broker/topic.h"

#include <gtest/gtest.h>
#include <score/stop_token.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
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
constexpr std::chrono::seconds kInitTimeout{5};

// Init() runs on the JobRunner thread. Its call counter is atomic.
// Start() and Stop() run on the test thread. Start() publishes kPublishedValue.
class FakeSource final : public ProactiveMachine, public Producer<int>
{
  public:
    FakeSource(const std::string& name, std::vector<std::string>& events, bool init_result)
        : ProactiveMachine{name},
          events_{events},
          init_result_{init_result},
          init_calls_{0U},
          started_{false},
          callback_{}
    {
    }

    bool Init() override
    {
        ++init_calls_;
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

    std::size_t GetInitCallCount() const noexcept
    {
        return init_calls_.load();
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
    std::atomic<std::size_t> init_calls_;
    bool started_;
    std::function<void(const int&)> callback_;
};

class FakePeriodicSource final : public PeriodicMachine, public Producer<int>
{
  public:
    explicit FakePeriodicSource(std::function<void()> on_stop)
        : PeriodicMachine{"periodic_source", std::chrono::milliseconds{1}}, callback_{}, on_stop_{std::move(on_stop)}
    {
    }

    bool Init() override
    {
        return true;
    }

    void Stop() noexcept override
    {
        if (on_stop_)
        {
            on_stop_();
            on_stop_ = {};
        }
        PeriodicMachine::Stop();
    }

    void SetPublishCallback(std::function<void(const int&)> callback) override
    {
        callback_ = std::move(callback);
    }

  private:
    void PeriodicTask() noexcept override
    {
        Publish(kPublishedValue);
    }

    void Publish(const int& data) override
    {
        if (callback_)
        {
            callback_(data);
        }
    }

    std::function<void(const int&)> callback_;
    std::function<void()> on_stop_;
};

class FakeSink final : public ReactiveMachine, public Consumer<int>
{
  public:
    explicit FakeSink(const std::string& name, std::function<void()> on_message = {})
        : ReactiveMachine{name}, received_{}, on_message_{std::move(on_message)}
    {
    }

    bool Init() override
    {
        return true;
    }

    void OnMessage(int data) override
    {
        received_.push_back(data);
        if (on_message_)
        {
            on_message_();
        }
    }

    const std::vector<int>& Received() const noexcept
    {
        return received_;
    }

  private:
    std::vector<int> received_;
    std::function<void()> on_message_;
};

// Tests add their own machines.
class TestHandler final : public GenericTimebaseHandler<int>
{
  public:
    explicit TestHandler(std::string name) : GenericTimebaseHandler<int>{std::move(name)} {}

    using GenericTimebaseHandler<int>::Add;
    using GenericTimebaseHandler<int>::Publish;
    using GenericTimebaseHandler<int>::Subscribe;

  private:
    void CreateComponents() noexcept override {}
};

class HandlerWithComponents final : public GenericTimebaseHandler<int>
{
  public:
    explicit HandlerWithComponents(std::vector<std::string>& events)
        : GenericTimebaseHandler<int>{"with_components"}, events_{events}, sink_{}
    {
    }

    std::shared_ptr<FakeSink> Sink() const noexcept
    {
        return sink_;
    }

  private:
    void CreateComponents() noexcept override
    {
        const Topic topic{"values"};
        auto source = Add(std::make_shared<FakeSource>("source", events_, true), kInitTimeout);
        sink_ = Add(std::make_shared<FakeSink>("sink"), kInitTimeout);
        Publish(source, topic);
        Subscribe(sink_, topic);
    }

    std::vector<std::string>& events_;
    std::shared_ptr<FakeSink> sink_;
};

class GenericTimebaseHandlerTest : public ::testing::Test
{
  protected:
    // Calls RunOnce() until predicate is true or the timeout expires.
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
            handler.RunOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        return true;
    }

    score::cpp::stop_source stop_source_{};
    score::cpp::stop_token stop_token_{stop_source_.get_token()};
};

TEST_F(GenericTimebaseHandlerTest, StartInitializesMachinesWithoutStartingThem)
{
    std::vector<std::string> events;
    auto source = std::make_shared<FakeSource>("source", events, true);
    {
        TestHandler handler{"first_run"};
        handler.Add(source, kInitTimeout);
        handler.Initialize();
        EXPECT_EQ(source->GetInitCallCount(), 0U);

        handler.RunOnce();
        EXPECT_EQ(source->GetInitCallCount(), 0U);

        handler.Start(stop_token_);
        EXPECT_TRUE(events.empty());
    }

    EXPECT_EQ(source->GetInitCallCount(), 1U);
    EXPECT_EQ(events, (std::vector<std::string>{"stop:source"}));
}

TEST_F(GenericTimebaseHandlerTest, StartsProactiveMachinesInAddOrderAndStopsInReverse)
{
    std::vector<std::string> events;
    TestHandler handler{"start_order"};
    handler.Add(std::make_shared<FakeSource>("first", events, true), kInitTimeout);
    auto second = handler.Add(std::make_shared<FakeSource>("second", events, true), kInitTimeout);
    handler.Initialize();
    handler.Start(stop_token_);

    ASSERT_TRUE(RunUntil(
        handler,
        [&second] {
            return second->IsStarted();
        },
        std::chrono::seconds{5}));
    EXPECT_EQ(events, (std::vector<std::string>{"start:first", "start:second"}));

    handler.Stop();
    EXPECT_EQ(events, (std::vector<std::string>{"start:first", "start:second", "stop:second", "stop:first"}));

    handler.Stop();
    EXPECT_EQ(events,
              (std::vector<std::string>{
                  "start:first", "start:second", "stop:second", "stop:first", "stop:second", "stop:first"}));
}

TEST_F(GenericTimebaseHandlerTest, DoesNotRestartWorkingMachinesWhenPolled)
{
    std::vector<std::string> events;
    TestHandler handler{"working"};
    auto source = handler.Add(std::make_shared<FakeSource>("source", events, true), kInitTimeout);
    handler.Initialize();
    handler.Start(stop_token_);

    ASSERT_TRUE(RunUntil(
        handler,
        [&source] {
            return source->IsStarted();
        },
        kInitTimeout));

    handler.RunOnce();
    handler.RunOnce();
    handler.RunOnce();

    EXPECT_EQ(source->GetInitCallCount(), 1U);
    EXPECT_EQ(events, (std::vector<std::string>{"start:source"}));
}

TEST_F(GenericTimebaseHandlerTest, DoesNotInitializeOrStartMachinesWhenStoppedBeforeStart)
{
    std::vector<std::string> events;
    auto source = std::make_shared<FakeSource>("source", events, true);
    {
        TestHandler handler{"stop_before_run"};
        handler.Add(source, kInitTimeout);
        handler.Initialize();

        handler.Stop();
        handler.RunOnce();
        handler.RunOnce();
        handler.RunOnce();

        EXPECT_EQ(events, (std::vector<std::string>{"stop:source"}));
    }

    EXPECT_EQ(source->GetInitCallCount(), 0U);
    EXPECT_EQ(events, (std::vector<std::string>{"stop:source", "stop:source"}));
}

TEST_F(GenericTimebaseHandlerTest, DoesNotRestartMachinesWhenPolledAfterStop)
{
    std::vector<std::string> events;
    TestHandler handler{"stop_after_start"};
    auto source = handler.Add(std::make_shared<FakeSource>("source", events, true), kInitTimeout);
    handler.Initialize();
    handler.Start(stop_token_);

    ASSERT_TRUE(RunUntil(
        handler,
        [&source] {
            return source->IsStarted();
        },
        kInitTimeout));

    handler.Stop();
    handler.RunOnce();
    handler.RunOnce();
    handler.RunOnce();

    EXPECT_EQ(source->GetInitCallCount(), 1U);
    EXPECT_EQ(events, (std::vector<std::string>{"start:source", "stop:source"}));
}

TEST_F(GenericTimebaseHandlerTest, DoesNotStartMachinesWhenStoppedDuringInitialization)
{
    std::vector<std::string> events;
    TestHandler handler{"stop_during_init"};
    auto source = handler.Add(std::make_shared<FakeSource>("source", events, false), kInitTimeout);
    handler.Initialize();
    handler.Start(stop_token_);

    const auto init_started = RunUntil(
        handler,
        [&source] {
            return source->GetInitCallCount() > 0U;
        },
        kInitTimeout);
    stop_source_.request_stop();
    ASSERT_TRUE(init_started);
    EXPECT_TRUE(events.empty());

    handler.Stop();
    handler.RunOnce();
    handler.RunOnce();
    handler.RunOnce();

    EXPECT_EQ(events, (std::vector<std::string>{"stop:source"}));
    EXPECT_FALSE(handler.IsFailed());
}

TEST_F(GenericTimebaseHandlerTest, DoesNotReportFailureWhenStopRequestedBeforePollingFailedInitialization)
{
    std::vector<std::string> events;
    TestHandler handler{"stop_then_poll"};
    handler.Add(std::make_shared<FakeSource>("source", events, false), kInitTimeout);
    handler.Initialize();
    handler.Start(stop_token_);
    stop_source_.request_stop();

    // The job runner reports kFailed for a cancelled initialization; poll until it does.
    for (int i = 0; i < 200 && !events.size(); ++i)
    {
        handler.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_FALSE(handler.IsFailed());
}

TEST_F(GenericTimebaseHandlerTest, DeliversDataOfPublisherToSubscribersOfTheSameTopic)
{
    std::vector<std::string> events;
    TestHandler handler{"delivery"};
    const Topic topic{"values"};
    auto first_source = handler.Add(std::make_shared<FakeSource>("first", events, true), kInitTimeout);
    auto second_source = handler.Add(std::make_shared<FakeSource>("second", events, true), kInitTimeout);
    auto sink = handler.Add(std::make_shared<FakeSink>("sink"), kInitTimeout);
    handler.Publish(first_source, topic);
    handler.Publish(second_source, topic);
    handler.Subscribe(sink, topic);
    handler.Initialize();
    handler.Start(stop_token_);

    ASSERT_TRUE(RunUntil(
        handler,
        [&second_source] {
            return second_source->IsStarted();
        },
        std::chrono::seconds{5}));
    EXPECT_EQ(sink->Received(), (std::vector<int>{kPublishedValue, kPublishedValue}));
}

TEST_F(GenericTimebaseHandlerTest, DisconnectsRetainedProducerAfterRepeatedStopAndDestruction)
{
    std::vector<std::string> events;
    auto source = std::make_shared<FakeSource>("source", events, true);
    auto sink = std::make_shared<FakeSink>("sink");
    {
        TestHandler handler{"retained_producer"};
        const Topic topic{"values"};
        handler.Add(source, kInitTimeout);
        handler.Add(sink, kInitTimeout);
        handler.Publish(source, topic);
        handler.Subscribe(sink, topic);
        handler.Initialize();
        handler.Start(stop_token_);

        ASSERT_TRUE(RunUntil(
            handler,
            [&source] {
                return source->IsStarted();
            },
            kInitTimeout));
        EXPECT_EQ(sink->Received(), std::vector<int>{kPublishedValue});

        handler.Stop();
        handler.Stop();
    }

    source->Start();
    EXPECT_EQ(sink->Received(), std::vector<int>{kPublishedValue});
    source->Stop();
}

TEST_F(GenericTimebaseHandlerTest, DestructionWaitsForPublicationInProgress)
{
    std::promise<void> publication_started;
    auto publication_started_future = publication_started.get_future();
    std::promise<void> release_publication;
    auto publication_released = release_publication.get_future();
    std::promise<void> stop_started;
    auto stop_started_future = stop_started.get_future();
    std::promise<void> destruction_finished;
    auto destruction_finished_future = destruction_finished.get_future();
    std::atomic<bool> publication_entered{false};

    auto source = std::make_shared<FakePeriodicSource>([&stop_started] {
        stop_started.set_value();
    });
    auto sink = std::make_shared<FakeSink>("blocking_sink",
                                           [&publication_started, &publication_released, &publication_entered] {
                                               if (!publication_entered.exchange(true))
                                               {
                                                   publication_started.set_value();
                                                   publication_released.wait();
                                               }
                                           });
    auto handler = std::make_unique<TestHandler>("active_publication");
    const Topic topic{"values"};
    handler->Add(source, kInitTimeout);
    handler->Add(sink, kInitTimeout);
    handler->Publish(source, topic);
    handler->Subscribe(sink, topic);
    handler->Initialize();
    handler->Start(stop_token_);

    const auto publication_is_started = RunUntil(
        *handler,
        [&publication_started_future] {
            return publication_started_future.wait_for(std::chrono::milliseconds{0}) == std::future_status::ready;
        },
        kInitTimeout);
    EXPECT_TRUE(publication_is_started);
    if (!publication_is_started)
    {
        release_publication.set_value();
        return;
    }

    std::thread destroyer([&handler, &destruction_finished] {
        handler.reset();
        destruction_finished.set_value();
    });
    EXPECT_EQ(stop_started_future.wait_for(kInitTimeout), std::future_status::ready);
    EXPECT_EQ(destruction_finished_future.wait_for(std::chrono::milliseconds{50}), std::future_status::timeout);

    release_publication.set_value();
    destroyer.join();

    EXPECT_EQ(destruction_finished_future.wait_for(std::chrono::milliseconds{0}), std::future_status::ready);
    ASSERT_FALSE(sink->Received().empty());
    EXPECT_EQ(sink->Received().front(), kPublishedValue);
}

TEST_F(GenericTimebaseHandlerTest, RejectsPublishingTheSameProducerTwice)
{
    std::vector<std::string> events;
    TestHandler handler{"duplicate_producer"};
    const Topic topic{"first"};
    auto source = handler.Add(std::make_shared<FakeSource>("source", events, true), kInitTimeout);
    handler.Publish(source, topic);

    EXPECT_DEATH(handler.Publish(source, topic), "");
    EXPECT_DEATH(handler.Publish(source, Topic{"second"}), "");
}

TEST_F(GenericTimebaseHandlerTest, RejectsPublishingTheSameExpiredProducerTwice)
{
    std::vector<std::string> events;
    TestHandler handler{"expired_producer"};
    std::weak_ptr<Producer<int>> producer;
    {
        auto source = std::make_shared<FakeSource>("source", events, true);
        producer = source;
        handler.Publish(producer, Topic{"first"});
    }

    ASSERT_TRUE(producer.expired());
    EXPECT_DEATH(handler.Publish(producer, Topic{"second"}), "");
}

TEST_F(GenericTimebaseHandlerTest, DoesNotStartProactiveMachinesWhenInitializationTimesOut)
{
    std::vector<std::string> events;
    TestHandler handler{"init_timeout"};
    auto source = handler.Add(std::make_shared<FakeSource>("source", events, false), std::chrono::seconds{1});
    handler.Initialize();
    handler.Start(stop_token_);

    EXPECT_FALSE(RunUntil(
        handler,
        [&source] {
            return source->IsStarted();
        },
        std::chrono::seconds{2}));
    EXPECT_EQ(events, (std::vector<std::string>{"stop:source"}));
    EXPECT_TRUE(handler.IsFailed());

    const auto init_calls = source->GetInitCallCount();
    handler.RunOnce();
    handler.RunOnce();
    handler.RunOnce();

    EXPECT_EQ(source->GetInitCallCount(), init_calls);
    EXPECT_EQ(events, (std::vector<std::string>{"stop:source"}));

    handler.Stop();
    handler.RunOnce();
    EXPECT_EQ(events, (std::vector<std::string>{"stop:source", "stop:source"}));
}

TEST_F(GenericTimebaseHandlerTest, AppliesTheWiringDeclaredInCreateComponents)
{
    std::vector<std::string> events;
    HandlerWithComponents handler{events};
    handler.Initialize();
    handler.Start(stop_token_);
    const auto sink = handler.Sink();

    ASSERT_TRUE(RunUntil(
        handler,
        [&sink] {
            return !sink->Received().empty();
        },
        std::chrono::seconds{5}));
    EXPECT_EQ(sink->Received(), std::vector<int>{kPublishedValue});
}

}  // namespace
}  // namespace td
}  // namespace score
