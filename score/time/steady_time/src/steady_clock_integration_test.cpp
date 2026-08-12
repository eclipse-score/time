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
#include "score/time/steady_time/src/steady_clock.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace score
{
namespace time
{

TEST(SteadyClockTest, GetInstanceNowIsMonotonicallyIncreasing)
{
    const auto first = SteadyClock::GetInstance().Now();
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
    EXPECT_GT(SteadyClock::GetInstance().Now().TimePoint(), first.TimePoint());
}

}  // namespace time
}  // namespace score
