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
#include "score/time/high_res_steady_time/src/details/system_clock/high_res_steady_clock_backend_impl.h"

#include "score/time/high_res_steady_time/src/high_res_steady_time.h"
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

#include <chrono>
#include <memory>

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace sys_time
{

HighResSteadyClockBackendImpl::HighResSteadyClockBackendImpl() noexcept  = default;
HighResSteadyClockBackendImpl::~HighResSteadyClockBackendImpl() noexcept = default;

ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus> HighResSteadyClockBackendImpl::Now() const noexcept
{
    const auto raw = std::chrono::high_resolution_clock::now().time_since_epoch();
    const HighResSteadyTime::Timepoint tp{std::chrono::duration_cast<HighResSteadyTime::Duration>(raw)};
    return ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>{tp, NoStatus{}};
}

}  // namespace sys_time
}  // namespace high_res_steady_time

template <>
std::shared_ptr<HighResSteadyClockBackend> detail::CreateBackend<HighResSteadyTime>()
{
    return std::make_shared<high_res_steady_time::sys_time::HighResSteadyClockBackendImpl>();
}

}  // namespace time
}  // namespace score
