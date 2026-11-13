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
#include "score/time/high_res_steady_time/src/details/qtime/high_res_steady_qclock.h"

#include <gtest/gtest.h>
#include <time.h>
#include <cstdint>

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace qtime
{
namespace test
{

namespace
{
std::int64_t MonotonicNs()
{
    ::timespec ts{};
    EXPECT_EQ(::clock_gettime(CLOCK_MONOTONIC, &ts), 0);
    return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
}
}  // namespace

TEST(HighResSteadyQClockTest, NowIsBasedOnClockMonotonic)
{
    RecordProperty("Description",
                   "Regression test for GH #310: Now() must be on the CLOCK_MONOTONIC timeline used by the time "
                   "slave for local_time, i.e. lie between two direct CLOCK_MONOTONIC reads.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Error guessing based on knowledge or experience");
    RecordProperty("ASIL", "QM");

    HighResSteadyQClock clock;
    const auto before = MonotonicNs();
    const auto now = clock.Now().TimePointNs().count();
    const auto after = MonotonicNs();

    EXPECT_GE(now, before);
    EXPECT_LE(now, after);
}

}  // namespace test
}  // namespace qtime
}  // namespace high_res_steady_time
}  // namespace time
}  // namespace score
