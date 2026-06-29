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
#include "score/time/system_time/src/details/stub_impl/system_clock_backend_impl.h"
#include "score/time/system_time/src/system_clock.h"

#include <memory>

namespace score
{
namespace time
{

template <>
std::shared_ptr<SystemClockBackend> detail::CreateBackend<std::chrono::system_clock>()
{
    return std::make_shared<detail::SystemClockBackendImpl>();
}

}  // namespace time
}  // namespace score
