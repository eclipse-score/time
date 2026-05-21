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
#ifndef SCORE_TIMEDAEMON_CODE_COMMON_DATA_FLOW_CONSUMER_H
#define SCORE_TIMEDAEMON_CODE_COMMON_DATA_FLOW_CONSUMER_H

#include <functional>
#include <string>

namespace score
{
namespace td
{

/**
 * @brief Interface for components that consume time information data.
 *
 * The Consumer interface defines a contract for components that process
 * time information and may perform actions based on it
 */
template <class T>
class Consumer
{
  public:
    virtual ~Consumer() = default;

    /**
     * @brief Process the received time information.
     *
     * This method is responsible for processing the time information data.
     *
     * @param data The time information data to be processed
     */
    virtual void OnMessage(T data) = 0;

  protected:
    Consumer() = default;
    Consumer(const Consumer&) = default;
    Consumer& operator=(const Consumer&) = default;
    Consumer(Consumer&&) = default;
    Consumer& operator=(Consumer&&) = default;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_COMMON_DATA_FLOW_CONSUMER_H
