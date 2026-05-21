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
#ifndef SCORE_TIMEDAEMON_CODE_COMMON_MACHINES_BASE_MACHINE_H
#define SCORE_TIMEDAEMON_CODE_COMMON_MACHINES_BASE_MACHINE_H

#include <string>

namespace score
{
namespace td
{

/**
 * @brief Base class for all machine components in the VehicleTimeDaemon.
 *
 * BaseMachine serves as the foundation for all machine components,
 * implementing both ISubscriber and IProducer interfaces. It provides
 * common functionality for handling messages and publishing data.
 */
class BaseMachine
{
  public:
    /**
     * @brief Constructs a BaseMachine object with the specified machine name.
     *
     * @param name The name of the machine.
     */
    explicit BaseMachine(const std::string& name);

    virtual ~BaseMachine() = default;

    inline std::string GetName() const noexcept
    {
        return name_;
    }

    /**
     * @brief Pure virtual to initialize machine
     *
     * @return initialization status
     **/
    virtual bool Init() = 0;

  protected:
    BaseMachine(const BaseMachine& other) = delete;
    BaseMachine& operator=(const BaseMachine& other) = delete;
    BaseMachine(BaseMachine&& other) noexcept = delete;
    BaseMachine& operator=(BaseMachine&& other) noexcept = delete;

  private:
    const std::string name_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_COMMON_MACHINES_BASE_MACHINE_H
