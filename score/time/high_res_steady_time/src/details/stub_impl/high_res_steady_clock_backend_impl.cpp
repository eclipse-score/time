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
#include "score/time/high_res_steady_time/src/details/stub_impl/high_res_steady_clock_backend_impl.h"
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

#include <memory>

namespace score
{
namespace time
{

template <>
std::shared_ptr<HighResSteadyClockBackend> detail::CreateBackend<HighResSteadyTime>()
{
    return std::make_shared<detail::HighResSteadyClockBackendImpl>();
}

}  // namespace time
}  // namespace score
