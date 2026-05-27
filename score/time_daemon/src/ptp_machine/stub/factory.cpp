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
#include "score/time_daemon/src/ptp_machine/stub/factory.h"
#include "score/time/HighPrecisionLocalSteadyClock/details/factory_impl.h"

namespace score
{
namespace td
{

std::shared_ptr<GPTPStubMachine> CreateGPTPStubMachine(const std::string& name)
{
    constexpr std::chrono::milliseconds updateInterval(50);
    score::time::HighPrecisionLocalSteadyClock::FactoryImpl clockFactory{};
    auto clock = clockFactory.CreateHighPrecisionLocalSteadyClock();
    return std::make_shared<GPTPStubMachine>(name, updateInterval, std::move(clock));
}

}  // namespace td
}  // namespace score
