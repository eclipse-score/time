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

#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"
#include "score/time/high_res_steady_time/src/high_res_steady_time.h"

#include <gtest/gtest.h>
#include <time.h>
#include <chrono>
#include <cstdint>
#include <thread>

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace sys_time
{
namespace test
{

class HighResSteadyClockBackendImplTest : public ::testing::Test
{
};

TEST_F(HighResSteadyClockBackendImplTest, TimePointIsNotNegative)
{
    RecordProperty("Description",
                   "Verifies that HighResSteadyClockBackendImpl::Now() returns a non-negative time point "
                   "and that the clock is strictly monotonically increasing.");
    RecordProperty("Verifies", "score::time::high_res_steady_time::sys_time::HighResSteadyClockBackendImpl::Now()");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Error guessing based on knowledge or experience");
    RecordProperty("ASIL", "QM");

    HighResSteadyClockBackendImpl clock;
    const auto snapshot = clock.Now();

    ASSERT_GE(snapshot.TimePointNs().count(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});

    EXPECT_GT(clock.Now().TimePoint(), snapshot.TimePoint());
}

std::int64_t ReadNs(const clockid_t id)
{
    ::timespec ts{};
    EXPECT_EQ(::clock_gettime(id, &ts), 0);
    return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
}

TEST_F(HighResSteadyClockBackendImplTest, NowIsBasedOnClockMonotonic)
{
    RecordProperty("Description",
                   "Regression test for GH #310: Now() must share CLOCK_MONOTONIC's epoch (as used by the time "
                   "slave for local_time), not CLOCK_REALTIME's. The reading must lie between two direct "
                   "CLOCK_MONOTONIC reads and be far from CLOCK_REALTIME.");
    RecordProperty("Verifies", "score::time::high_res_steady_time::sys_time::HighResSteadyClockBackendImpl::Now()");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Error guessing based on knowledge or experience");
    RecordProperty("ASIL", "QM");

    HighResSteadyClockBackendImpl clock;

    const auto before = ReadNs(CLOCK_MONOTONIC);
    const auto now = clock.Now().TimePointNs().count();
    const auto after = ReadNs(CLOCK_MONOTONIC);

    EXPECT_GE(now, before);
    EXPECT_LE(now, after);
    // Realtime is ~decades since 1970, monotonic is uptime; they can never be this close.
    EXPECT_GT(ReadNs(CLOCK_REALTIME) - now, std::int64_t{365} * 24 * 3600 * 1'000'000'000);
}

TEST_F(HighResSteadyClockBackendImplTest, CreateBackendReturnsHighResSteadyClockBackendImpl)
{
    RecordProperty("Description",
                   "Verifies that detail::CreateBackend<HighResSteadyTime>() returns a HighResSteadyClockBackendImpl.");
    RecordProperty("Verifies", "score::time::detail::CreateBackend<HighResSteadyTime>()");
    RecordProperty("TestType", "Interface test");
    RecordProperty("ASIL", "QM");
    RecordProperty("PartiallyVerifies", "comp_req__time__supported_platforms");

    auto backend = detail::CreateBackend<HighResSteadyTime>();
    ASSERT_NE(backend, nullptr);
    EXPECT_NE(dynamic_cast<HighResSteadyClockBackendImpl*>(backend.get()), nullptr);
}

}  // namespace test
}  // namespace sys_time
}  // namespace high_res_steady_time
}  // namespace time
}  // namespace score
