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
#include "score/time/hirs_time/src/details/qtime/hirs_qclock.h"
#include "score/time/hirs_time/src/hirs_clock.h"

#include <gtest/gtest.h>

namespace score
{
namespace time
{
namespace hirs_time
{
namespace qtime
{
namespace
{

TEST(HirsQClockFactoryTest, CreateBackendReturnsNonNull)
{
    RecordProperty("Description",
                   "Verifies that detail::CreateBackend<HirsTime>() returns a valid shared_ptr.");
    RecordProperty("Verifies", "score::time::detail::CreateBackend<HirsTime>()");
    RecordProperty("TestType", "Interface test");
    RecordProperty("ASIL", "QM");

    auto backend = detail::CreateBackend<HirsTime>();
    ASSERT_NE(backend, nullptr);
}

TEST(HirsQClockFactoryTest, CreateBackendReturnsHirsQClock)
{
    RecordProperty("Description",
                   "Verifies that detail::CreateBackend<HirsTime>() returns an HirsQClock instance.");
    RecordProperty("ASIL", "QM");

    auto backend = detail::CreateBackend<HirsTime>();
    ASSERT_NE(backend, nullptr);
    EXPECT_NE(dynamic_cast<HirsQClock*>(backend.get()), nullptr);
}

}  // namespace
}  // namespace qtime
}  // namespace hirs_time
}  // namespace time
}  // namespace score
