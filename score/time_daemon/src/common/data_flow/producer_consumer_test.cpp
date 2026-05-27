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
#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/data_flow/producer.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <map>

using ::testing::_;

namespace score
{
namespace td
{
namespace test
{

struct FakeTimeInfo
{
    uint64_t ptp_assumed_time;
    uint64_t local_time;
};

bool operator==(const FakeTimeInfo& first, const FakeTimeInfo& second) noexcept
{
    const bool same_local = (first.local_time == second.local_time);
    const bool same_ptp = (first.ptp_assumed_time == second.ptp_assumed_time);

    return (same_local && same_ptp);
}

class FakeProducerMachine : public Producer<FakeTimeInfo>
{
  public:
    FakeProducerMachine() = default;
    ~FakeProducerMachine() override = default;

    void SetPublishCallback(std::function<void(const FakeTimeInfo&)> callback) override
    {
        publishCallback_ = std::move(callback);
    }

    void Publish(const FakeTimeInfo& data) override
    {
        if (publishCallback_)
        {
            publishCallback_(data);
        }
    }

  private:
    std::function<void(const FakeTimeInfo&)> publishCallback_;
};

class FakeConsumerMachine : public Consumer<FakeTimeInfo>
{
  public:
    FakeConsumerMachine() = default;
    ~FakeConsumerMachine() override = default;

    MOCK_METHOD(void, OnMessage, (FakeTimeInfo), (override));
};

class FakeConsumerProducerMachine : public Consumer<FakeTimeInfo>, public Producer<FakeTimeInfo>
{
  public:
    FakeConsumerProducerMachine() = default;
    ~FakeConsumerProducerMachine() override = default;

    // Producer interface
    void SetPublishCallback(std::function<void(const FakeTimeInfo&)> callback) override
    {
        publishCallback_ = std::move(callback);
    }

    void Publish(const FakeTimeInfo& data) override
    {
        if (publishCallback_)
        {
            publishCallback_(data);
        }
    }

    // Consumer interface
    virtual void OnMessage(FakeTimeInfo data) override
    {
        Publish(std::move(data));
    }

  private:
    std::function<void(const FakeTimeInfo&)> publishCallback_;
};

class FakeMessageBroker
{
  public:
    void RegisterSubscriber(const std::string& topic, Consumer<FakeTimeInfo>* consumer)
    {
        subscribers_[topic] = consumer;
    }
    void Publish(const std::string& topic, const FakeTimeInfo& data)
    {
        auto it = subscribers_.find(topic);
        if (it != subscribers_.end())
        {
            it->second->OnMessage(data);
        }
    }

  private:
    std::map<std::string, Consumer<FakeTimeInfo>*> subscribers_;
};

class FakeSubscriptionManager
{
  public:
    FakeSubscriptionManager(FakeMessageBroker& broker) : messageBroker_(broker) {}

    // Register a consumer to receive messages on a topic
    void Subscribe(const std::string& topic, Consumer<FakeTimeInfo>* consumer)
    {
        messageBroker_.RegisterSubscriber(topic, consumer);
    }

    // Register a producer for a specific topic
    void RegisterProducer(const std::string& topic, Producer<FakeTimeInfo>* producer)
    {
        // Set up the callback that the producer will use when publishing
        producer->SetPublishCallback([this, topic](const FakeTimeInfo& data) {
            messageBroker_.Publish(topic, data);
        });
    }

  private:
    FakeMessageBroker& messageBroker_;
};

}  // namespace test

class ProducerConsumerTest : public ::testing::Test
{
  public:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ProducerConsumerTest, TestProducerConsumerNotification)
{
    test::FakeMessageBroker messageBroker;
    test::FakeSubscriptionManager subManager(messageBroker);
    test::FakeConsumerMachine timeConsumer;
    test::FakeProducerMachine timeProducer;
    test::FakeProducerMachine offTopicProducer;

    subManager.RegisterProducer("TimeTopic", &timeProducer);
    subManager.RegisterProducer("OffTopic", &offTopicProducer);

    subManager.Subscribe("TimeTopic", &timeConsumer);

    test::FakeTimeInfo testData = {1234567890, 9876543210};

    EXPECT_CALL(timeConsumer, OnMessage(testData)).Times(1);

    timeProducer.Publish(testData);
}

TEST_F(ProducerConsumerTest, TestProducerConsumerNoNotification)
{
    test::FakeMessageBroker messageBroker;
    test::FakeSubscriptionManager subManager(messageBroker);
    test::FakeConsumerMachine timeConsumer;
    test::FakeProducerMachine timeProducer;
    test::FakeProducerMachine offTopicProducer;

    subManager.RegisterProducer("TimeTopic", &timeProducer);
    subManager.RegisterProducer("OffTopic", &offTopicProducer);

    subManager.Subscribe("TimeTopic", &timeConsumer);

    test::FakeTimeInfo testData = {0, 0};

    EXPECT_CALL(timeConsumer, OnMessage(_)).Times(0);

    offTopicProducer.Publish(testData);
}

TEST_F(ProducerConsumerTest, TestProducerConsumerNotificationChain)
{
    test::FakeMessageBroker messageBroker;
    test::FakeSubscriptionManager subManager(messageBroker);
    test::FakeProducerMachine initialProducer;
    test::FakeConsumerProducerMachine intermediateConsumerProducer1;
    test::FakeConsumerProducerMachine intermediateConsumerProducer2;
    test::FakeConsumerMachine finalConsumer;

    subManager.RegisterProducer("TimeTopic", &initialProducer);
    subManager.RegisterProducer("TimeTopic1", &intermediateConsumerProducer1);
    subManager.RegisterProducer("TimeTopic2", &intermediateConsumerProducer2);

    subManager.Subscribe("TimeTopic", &intermediateConsumerProducer1);
    subManager.Subscribe("TimeTopic1", &intermediateConsumerProducer2);
    subManager.Subscribe("TimeTopic2", &finalConsumer);

    test::FakeTimeInfo testData = {1234567890, 9876543210};

    EXPECT_CALL(finalConsumer, OnMessage(testData)).Times(1);

    initialProducer.Publish(testData);
}

}  // namespace td
}  // namespace score
