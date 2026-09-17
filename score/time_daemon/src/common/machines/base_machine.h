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
#ifndef SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_BASE_MACHINE_H
#define SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_BASE_MACHINE_H

#include <string>

namespace score::td
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
    explicit BaseMachine(std::string name);

    virtual ~BaseMachine() = default;

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    std::string GetName() const noexcept
    {
        return name_;
    }

    /**
     * @brief Pure virtual to initialize machine
     *
     * @return initialization status
     **/
    virtual bool Init() = 0;

    // Kept public and deleted (not protected) so misuse fails with a clear "call to deleted
    // function" diagnostic instead of a confusing "is protected within this context" one.
    BaseMachine(const BaseMachine& other) = delete;
    BaseMachine& operator=(const BaseMachine& other) = delete;
    BaseMachine(BaseMachine&& other) noexcept = delete;
    BaseMachine& operator=(BaseMachine&& other) noexcept = delete;

  private:
    const std::string name_;
};

}  // namespace score::td

#endif  // SCORE_TIME_DAEMON_SRC_COMMON_MACHINES_BASE_MACHINE_H
