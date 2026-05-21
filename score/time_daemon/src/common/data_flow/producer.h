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
#ifndef SCORE_TIMEDAEMON_CODE_COMMON_DATA_FLOW_PRODUCER_H
#define SCORE_TIMEDAEMON_CODE_COMMON_DATA_FLOW_PRODUCER_H

#include <functional>

namespace score
{
namespace td
{

/**
 * @brief Interface for components that produce and publish time data.
 *
 * The Producer interface defines a contract for components that generate
 * time information and need to publish it to other components.
 */
template <class T>
class Producer
{
  public:
    virtual ~Producer() = default;

    /**
     * @brief Sets the callback function to be invoked when publishing data.
     *
     * This method allows external components (typically the message broker)
     * to register a callback that will be invoked when this producer publishes
     * data. This facilitates decoupling the producer from knowledge of the
     * message distribution mechanism.
     *
     * @param callback Function to be called when data is published
     */
    virtual void SetPublishCallback(std::function<void(const T&)> callback) = 0;

  protected:
    Producer() = default;
    Producer(const Producer&) = default;
    Producer& operator=(const Producer&) = default;
    Producer(Producer&&) = default;
    Producer& operator=(Producer&&) = default;

    /**
     * @brief Publishes the time information data using the registered callback.
     *
     * This method invokes the previously set publish callback to distribute
     * the data to interested consumers. It is typically
     * called by the producer when new data is available.
     *
     * @param data The data to be published
     */
    virtual void Publish(const T& data) = 0;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_COMMON_DATA_FLOW_PRODUCER_H
