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
#include "score/time/steady_time/src/details/steady_time_impl/steady_clock_backend_impl.h"

#include <gtest/gtest.h>

#include <thread>

namespace score
{
namespace time
{
namespace detail
{
namespace test
{

class SteadyClockBackendImplTest : public ::testing::Test
{
};

TEST_F(SteadyClockBackendImplTest, NowReturnsNonNegativeTimePoint)
{
    SteadyClockBackendImpl clock;
    const auto snapshot = clock.Now();

    EXPECT_GE(snapshot.TimePointNs().count(), 0);
}

TEST_F(SteadyClockBackendImplTest, NowIsMonotonicallyIncreasing)
{
    SteadyClockBackendImpl clock;
    const auto first = clock.Now();

    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    EXPECT_GT(clock.Now().TimePoint(), first.TimePoint());
}

TEST_F(SteadyClockBackendImplTest, NowSnapshotCarriesNoStatus)
{
    SteadyClockBackendImpl clock;
    const auto snapshot = clock.Now();

    // NoStatus is an empty struct — verify it is accessible (compile + link check).
    const NoStatus status = snapshot.Status();
    (void)status;
    SUCCEED();
}

}  // namespace test
}  // namespace detail
}  // namespace time
}  // namespace score
