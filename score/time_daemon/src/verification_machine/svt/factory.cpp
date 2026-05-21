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
#include "score/time_daemon/src/verification_machine/svt/factory.h"
#include "score/time_daemon/src/verification_machine/svt/validators/synchronization_validator.h"
#include "score/time_daemon/src/verification_machine/svt/validators/time_jumps_validator.h"
#include "score/time_daemon/src/verification_machine/svt/validators/timeout_validator.h"
#include "score/time/HighPrecisionLocalSteadyClock/details/factory_impl.h"

namespace score
{
namespace td
{

std::shared_ptr<SvtVerificationMachine> CreateSvtVerificationMachine(const std::string& name)
{
    const auto hplsc_factory = score::time::HighPrecisionLocalSteadyClock::FactoryImpl();
    auto machine = std::make_shared<SvtVerificationMachine>(
        name,
        []() {
            return std::make_unique<SynchronizationValidator>(/*args for validation*/);
        },
        [&hplsc_factory]() {
            return std::make_unique<TimeoutValidator>(hplsc_factory.CreateHighPrecisionLocalSteadyClock(),
                                                      std::chrono::nanoseconds{3'300'000'000});
        },
        [&hplsc_factory]() {
            return std::make_unique<TimeJumpsValidator>(hplsc_factory.CreateHighPrecisionLocalSteadyClock(),
                                                        std::chrono::nanoseconds(500'000),
                                                        std::chrono::nanoseconds(5'000'000'000),
                                                        2U);
        });

    return machine;
}

}  // namespace td
}  // namespace score
