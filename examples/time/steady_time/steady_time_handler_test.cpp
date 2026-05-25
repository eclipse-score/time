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
#include "examples/time/steady_time/steady_time_handler.h"

#include "score/time/steady_time/steady_clock_mock.h"
#include "score/time/clock/scoped_clock_override.h"
#include "score/time/clock/clock_snapshot.h"
#include "score/time/clock/no_status.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

using ::testing::Return;

namespace examples
{
namespace time
{
namespace steady_time
{
namespace test
{

class SteadyTimeHandlerTest : public ::testing::Test
{
  protected:
    SteadyTimeHandlerTest()
        : mock_{std::make_shared<score::time::SteadyClockMock>()}
        , guard_{mock_}
    {
    }

    std::shared_ptr<score::time::SteadyClockMock>                   mock_;
    score::time::test_utils::ScopedClockOverride<std::chrono::steady_clock>       guard_;
};

TEST_F(SteadyTimeHandlerTest, ReportContainsMonotonicTimeFromMock)
{
    const std::chrono::steady_clock::time_point tp{std::chrono::nanoseconds{3'000'000'000LL}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<std::chrono::steady_clock::time_point, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    SteadyTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.monotonic_ns, 3'000'000'000LL);
}

TEST_F(SteadyTimeHandlerTest, ReportContainsZeroForEpochTimepoint)
{
    const std::chrono::steady_clock::time_point tp{std::chrono::nanoseconds{0}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<std::chrono::steady_clock::time_point, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    SteadyTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.monotonic_ns, 0LL);
}

}  // namespace test
}  // namespace steady_time
}  // namespace time
}  // namespace examples
