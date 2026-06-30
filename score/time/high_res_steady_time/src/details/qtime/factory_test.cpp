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
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

#include <gtest/gtest.h>

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace qtime
{
namespace
{

TEST(HighResSteadyQClockFactoryTest, CreateBackendReturnsNonNull)
{
    RecordProperty("Description",
                   "Verifies that detail::CreateBackend<HighResSteadyTime>() returns a valid shared_ptr.");
    RecordProperty("Verifies", "score::time::detail::CreateBackend<HighResSteadyTime>()");
    RecordProperty("TestType", "Interface test");
    RecordProperty("ASIL", "QM");

    auto backend = detail::CreateBackend<HighResSteadyTime>();
    ASSERT_NE(backend, nullptr);
}

TEST(HighResSteadyQClockFactoryTest, CreateBackendReturnsHighResSteadyQClock)
{
    RecordProperty("Description",
                   "Verifies that detail::CreateBackend<HighResSteadyTime>() returns an HighResSteadyQClock instance.");
    RecordProperty("ASIL", "QM");

    auto backend = detail::CreateBackend<HighResSteadyTime>();
    ASSERT_NE(backend, nullptr);
    EXPECT_NE(dynamic_cast<HighResSteadyQClock*>(backend.get()), nullptr);
}

}  // namespace
}  // namespace qtime
}  // namespace high_res_steady_time
}  // namespace time
}  // namespace score
