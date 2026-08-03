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
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace score
{
namespace time
{

TEST(HighResSteadyClockTest, GetInstanceNowIsMonotonicallyIncreasing)
{
    const auto first = HighResSteadyClock::GetInstance().Now();
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
    EXPECT_GT(HighResSteadyClock::GetInstance().Now().TimePoint(), first.TimePoint());
}

}  // namespace time
}  // namespace score
