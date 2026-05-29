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
#include "score/time/hirs_time/src/details/system_clock/hirs_clock_backend_impl.h"

#include "score/time/hirs_time/src/hirs_time.h"
#include "score/time/hirs_time/src/hirs_clock.h"

#include <chrono>
#include <memory>

namespace score
{
namespace time
{
namespace hirs_time
{
namespace sys_time
{

HirsClockBackendImpl::HirsClockBackendImpl() noexcept  = default;
HirsClockBackendImpl::~HirsClockBackendImpl() noexcept = default;

ClockSnapshot<HirsTime::Timepoint, NoStatus> HirsClockBackendImpl::Now() const noexcept
{
    const auto raw = std::chrono::high_resolution_clock::now().time_since_epoch();
    const HirsTime::Timepoint tp{std::chrono::duration_cast<HirsTime::Duration>(raw)};
    return ClockSnapshot<HirsTime::Timepoint, NoStatus>{tp, NoStatus{}};
}

}  // namespace sys_time
}  // namespace hirs_time

template <>
std::shared_ptr<HirsClockBackend> detail::CreateBackend<HirsTime>()
{
    return std::make_shared<hirs_time::sys_time::HirsClockBackendImpl>();
}

}  // namespace time
}  // namespace score
