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
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"
#include "score/time_daemon/src/verification_machine/svt/svt_verification_machine.h"
#include "score/time_daemon/src/verification_machine/svt/validators/synchronization_validator.h"
#include "score/time_daemon/src/verification_machine/svt/validators/time_jumps_validator.h"
#include "score/time_daemon/src/verification_machine/svt/validators/timeout_validator.h"
#include <memory>
#include <string>

namespace score::td
{
namespace
{
constexpr auto kTimeoutThreshold = std::chrono::nanoseconds{3'300'000'000};
constexpr auto kTimeJumpThreshold = std::chrono::nanoseconds{500'000};
constexpr auto kSyncDebounceThreshold = std::chrono::nanoseconds{5'000'000'000};
constexpr auto kValidFramesThreshold = 2U;
}  // namespace

auto CreateSvtVerificationMachine(const std::string& name) -> std::shared_ptr<SvtVerificationMachine>
{
    auto machine = std::make_shared<SvtVerificationMachine>(
        name,
        []() -> auto {
            return std::make_unique<SynchronizationValidator>(/*args for validation*/);
        },
        []() -> auto {
            return std::make_unique<TimeoutValidator>(score::time::HighResSteadyClock::GetInstance(),
                                                      kTimeoutThreshold);
        },
        []() -> auto {
            return std::make_unique<TimeJumpsValidator>(score::time::HighResSteadyClock::GetInstance(),
                                                        kTimeJumpThreshold,
                                                        kValidFramesThreshold,
                                                        kSyncDebounceThreshold);
        });

    return machine;
}

}  // namespace score::td
