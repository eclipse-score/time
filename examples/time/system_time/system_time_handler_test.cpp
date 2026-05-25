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
#include "examples/time/system_time/system_time_handler.h"

#include "score/time/system_time/system_clock_mock.h"
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
namespace system_time
{
namespace test
{

class SystemTimeHandlerTest : public ::testing::Test
{
  protected:
    SystemTimeHandlerTest()
        : mock_{std::make_shared<score::time::SystemClockMock>()}
        , guard_{mock_}
    {
    }

    std::shared_ptr<score::time::SystemClockMock>                   mock_;
    score::time::test_utils::ScopedClockOverride<std::chrono::system_clock>      guard_;
};

TEST_F(SystemTimeHandlerTest, ReportContainsUnixTimeFromMock)
{
    // 2026-01-01 00:00:00 UTC in nanoseconds since Unix epoch
    constexpr std::int64_t kYear2026Ns = 1'767'225'600LL * 1'000'000'000LL;
    const std::chrono::system_clock::time_point tp{std::chrono::nanoseconds{kYear2026Ns}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<std::chrono::system_clock::time_point, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    SystemTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.unix_ns, kYear2026Ns);
}

TEST_F(SystemTimeHandlerTest, ReportContainsZeroForEpochTimepoint)
{
    const std::chrono::system_clock::time_point tp{std::chrono::nanoseconds{0}};
    EXPECT_CALL(*mock_, Now()).WillOnce(Return(
        score::time::ClockSnapshot<std::chrono::system_clock::time_point, score::time::NoStatus>{
            tp, score::time::NoStatus{}}));

    SystemTimeHandler handler;
    const TimeReport report = handler.GetCurrentTime();

    EXPECT_EQ(report.unix_ns, 0LL);
}

}  // namespace test
}  // namespace system_time
}  // namespace time
}  // namespace examples
