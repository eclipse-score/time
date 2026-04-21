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
#include "score/TimeDaemon/code/ptp_machine/shm/factory.h"

namespace score
{
namespace td
{

std::shared_ptr<GPTPShmMachine> CreateGPTPShmMachine(const std::string& name, const std::string& ipc_name)
{
    constexpr std::chrono::milliseconds updateInterval(50);
    return std::make_shared<GPTPShmMachine>(name, updateInterval, ipc_name);
}

}  // namespace td
}  // namespace score
