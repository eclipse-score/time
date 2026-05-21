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
#ifndef SCORE_TIMEDAEMON_CODE_MSG_BROKER_MSG_BROKER_H
#define SCORE_TIMEDAEMON_CODE_MSG_BROKER_MSG_BROKER_H

#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/data_flow/producer.h"
#include "score/time_daemon/src/msg_broker/subscription.h"
#include "score/time_daemon/src/msg_broker/topic.h"
#include "score/mw/log/logging.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace score
{
namespace td
{

///
/// \brief Class to store each subscriber for dedicated topic. Idea is simple, any component can use subscribe with
/// dedicated callback
///        for explicit "topic". Then callback will be invoked when some component will call OnNewData() for dedicated
///        topic.
///
template <typename T>
class MessageBroker : public std::enable_shared_from_this<MessageBroker<T>>
{
  public:
    void AddSubscriber(const Topic& topic, std::weak_ptr<Consumer<T>> subscriber_weak);
    void AddProducer(const Topic& topic, std::weak_ptr<Producer<T>> producer_weak);

  private:
    void Publish(const Topic& topic, const T& data) const;
    void Subscribe(const Topic& topic, const Subscription<T>& subscription);
    void OnNewData(const Topic& topic, const T& data) const;

    std::unordered_map<Topic, std::vector<Subscription<T>>> subscribers_;
};

template <typename T>
void MessageBroker<T>::AddSubscriber(const Topic& topic, std::weak_ptr<Consumer<T>> subscriber_weak)
{
    Subscribe(topic, Subscription<T>([subscriber_weak](const T& data) {
                  const auto subscriber = subscriber_weak.lock();
                  if (subscriber)
                  {
                      subscriber->OnMessage(data);
                  }
              }));
}

template <typename T>
void MessageBroker<T>::AddProducer(const Topic& topic, std::weak_ptr<Producer<T>> producer_weak)
{
    const auto producer = producer_weak.lock();
    if (producer)
    {
        std::weak_ptr<MessageBroker<T>> weak_broker = this->shared_from_this();

        producer->SetPublishCallback([weak_broker, topic](const T& data) {
            const auto broker = weak_broker.lock();
            if (broker)
            {
                broker->OnNewData(topic, data);
            }
        });
    }
}

template <typename T>
void MessageBroker<T>::Publish(const Topic& topic, const T& data) const
{
    auto it = subscribers_.find(topic);
    if (it != subscribers_.end())
    {
        for (auto& sub : it->second)
        {
            if (sub.callback_)
                sub.callback_(data);
        }
    }
}

template <typename T>
void MessageBroker<T>::Subscribe(const Topic& topic, const Subscription<T>& subscription)
{
    subscribers_[topic].push_back(subscription);
}

template <typename T>
void MessageBroker<T>::OnNewData(const Topic& topic, const T& data) const
{
    Publish(topic, data);
}

}  // namespace td
}  // namespace score

#endif  // #ifndef SCORE_TIMEDAEMON_CODE_MSG_BROKER_MSG_BROKER_H
