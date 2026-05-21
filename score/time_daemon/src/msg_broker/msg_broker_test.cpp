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
#include "score/time_daemon/src/msg_broker/msg_broker.h"
#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/data_flow/producer.h"

#include <gtest/gtest.h>
#include <memory>

namespace score
{
namespace td
{

template <typename T>
class MockConsumer : public Consumer<T>
{
  public:
    std::vector<T> received_data;

    void OnMessage(T data) override
    {
        received_data.push_back(data);
    }
};

template <typename T>
class MockProducer : public Producer<T>
{
  public:
    std::function<void(const T&)> publish_callback_;

    void SetPublishCallback(std::function<void(const T&)> callback) override
    {
        publish_callback_ = std::move(callback);
    }

    void Publish(const T& data) override
    {
        if (publish_callback_)
            publish_callback_(data);
    }

    void Produce(const T& data)
    {
        Publish(data);
    }
};

class MessageBrokerTest : public ::testing::Test
{
    void SetUp() override
    {
        broker = std::make_shared<MessageBroker<int>>();
    }

  protected:
    std::shared_ptr<MessageBroker<int>> broker;
};

TEST_F(MessageBrokerTest, SingleSubscriberReceivesData)
{
    auto consumer = std::make_shared<MockConsumer<int>>();
    broker->AddSubscriber(Topic("topic1"), consumer);

    auto producer = std::make_shared<MockProducer<int>>();
    broker->AddProducer(Topic("topic1"), producer);

    producer->Produce(42);

    ASSERT_EQ(consumer->received_data.size(), 1);
    EXPECT_EQ(consumer->received_data[0], 42);
}

TEST_F(MessageBrokerTest, MultipleSubscribersReceiveData)
{
    auto consumer1 = std::make_shared<MockConsumer<int>>();
    auto consumer2 = std::make_shared<MockConsumer<int>>();

    broker->AddSubscriber(Topic("topic1"), consumer1);
    broker->AddSubscriber(Topic("topic1"), consumer2);

    auto producer = std::make_shared<MockProducer<int>>();
    broker->AddProducer(Topic("topic1"), producer);

    producer->Produce(100);

    EXPECT_EQ(consumer1->received_data.size(), 1);
    EXPECT_EQ(consumer2->received_data.size(), 1);
    EXPECT_EQ(consumer1->received_data[0], 100);
    EXPECT_EQ(consumer2->received_data[0], 100);
}

TEST_F(MessageBrokerTest, NoSubscriberForOtherTopic)
{
    auto consumer = std::make_shared<MockConsumer<int>>();
    broker->AddSubscriber(Topic("topic1"), consumer);

    auto producer = std::make_shared<MockProducer<int>>();
    broker->AddProducer(Topic("topic2"), producer);

    producer->Produce(7);

    EXPECT_TRUE(consumer->received_data.empty());
}

TEST_F(MessageBrokerTest, MultipleDataProduction)
{
    auto consumer = std::make_shared<MockConsumer<int>>();
    broker->AddSubscriber(Topic("topic1"), consumer);

    auto producer = std::make_shared<MockProducer<int>>();
    broker->AddProducer(Topic("topic1"), producer);

    for (int i = 0; i < 5; ++i)
        producer->Produce(i);

    ASSERT_EQ(consumer->received_data.size(), 5);
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(consumer->received_data[i], i);
}

}  // namespace td
}  // namespace score
