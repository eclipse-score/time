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
#include "examples/time/hpls_time/hpls_time_handler.h"

#include "score/time/hpls_time/hpls_clock_mock.h"
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
namespace hpls_time
{
namespace test
{

class HplsTimeHandlerTest : public ::testing::Test
{
  protected:
    HplsTimeHandlerTest()
        : mock_{std::make_shared<score::time::HplsClockMock>()}
        , guard_{mock_}
    {
    }

    std::shared_ptr<score::time::HplsClockMock>          mock_;
    score::time::test_utils::ScopedClockOverride<score::time::HplsTime> guard_;
};

TEST_F(HplsTimeHandlerTest, ReportContainsTimePointFromMock)
{
    const score::time::HplsTime::Timepoint tp{std::chrono::nanoseconds{7'654'321'000LL}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HplsTime::Timepoint, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    HplsTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.time_ns, 7'654'321'000LL);
}

TEST_F(HplsTimeHandlerTest, ReportContainsZeroForEpochTimepoint)
{
    const score::time::HplsTime::Timepoint tp{std::chrono::nanoseconds{0}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<score::time::HplsTime::Timepoint, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    HplsTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.time_ns, 0LL);
}

}  // namespace test
}  // namespace hpls_time
}  // namespace time
}  // namespace examples
