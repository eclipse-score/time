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
#include "score/time/high_res_steady_time/src/details/qtime/tick_provider_mock.h"

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace qtime
{

// GetClockCyclesPerSec() routes to the mock when the mock is active.
std::uint64_t GetClockCyclesPerSec()
{
    return TickProviderMock::GetMockInstance().GetClockCyclesPerSec();
}

std::unique_ptr<TickProviderMock> TickProviderMock::mock_instance_{};
std::mutex TickProviderMock::mutex_{};

}  // namespace qtime
}  // namespace high_res_steady_time
}  // namespace time
}  // namespace score
