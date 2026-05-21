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
#ifndef SCORE_TIMEDAEMON_CODE_MSG_BROKER_SUBSCRIPTION_H
#define SCORE_TIMEDAEMON_CODE_MSG_BROKER_SUBSCRIPTION_H

#include <functional>

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
template <class T>
class Subscription
{
  public:
    Subscription() = default;

    explicit Subscription(std::function<void(const T&)> cb) : callback_(std::move(cb)) {}

    std::function<void(const T&)> callback_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_MSG_BROKER_SUBSCRIPTION_H
