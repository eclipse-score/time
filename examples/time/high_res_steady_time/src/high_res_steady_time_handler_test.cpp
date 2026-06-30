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
#include "examples/time/high_res_steady_time/src/high_res_steady_time_handler.h"

#include "score/time/high_res_steady_time/src/high_res_steady_clock_backend_mock.h"
#include "score/time/clock/src/scoped_clock_override.h"
#include "score/time/clock/src/clock_snapshot.h"
#include "score/time/clock/src/no_status.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

using ::testing::Return;

namespace examples
{
namespace time
{
namespace high_res_steady_time
{
namespace test
{

class HighResSteadyTimeHandlerTest : public ::testing::Test
{
  protected:
    HighResSteadyTimeHandlerTest()
        : mock_{std::make_shared<score::time::HighResSteadyClockBackendMock>()}
        , guard_{mock_}
    {
    }

    std::shared_ptr<score::time::HighResSteadyClockBackendMock> mock_;
    score::time::test_utils::ScopedClockOverride<score::time::HighResSteadyTime> guard_;
};

TEST_F(HighResSteadyTimeHandlerTest, ReportContainsTimePointFromMock)
{
    const score::time::HighResSteadyTime::Timepoint tp{std::chrono::nanoseconds{7'654'321'000LL}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HighResSteadyTime::Timepoint, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    HighResSteadyTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.time_ns, 7'654'321'000LL);
}

TEST_F(HighResSteadyTimeHandlerTest, ReportContainsZeroForEpochTimepoint)
{
    const score::time::HighResSteadyTime::Timepoint tp{std::chrono::nanoseconds{0}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HighResSteadyTime::Timepoint, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    HighResSteadyTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.time_ns, 0LL);
}

}  // namespace test
}  // namespace high_res_steady_time
}  // namespace time
}  // namespace examples
