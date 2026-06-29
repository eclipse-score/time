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
#include "score/time/clock/src/clock_status.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

namespace score
{
namespace time
{

class TestClockStatus : public ::testing::Test
{
  public:
    enum class StatusFlag : std::uint8_t
    {
        kTimeOut        = 0U,
        kSynchronized   = 1U,
        kSynchToGateway = 2U,
        kUnknown        = 3U,
    };
};

TEST_F(TestClockStatus, ObjectCreatedFromFlagList)
{
    ClockStatus<StatusFlag> status{StatusFlag::kSynchronized, StatusFlag::kSynchToGateway};

    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchronized));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchToGateway));
    EXPECT_FALSE(status.IsAnyOfFlagsActive({StatusFlag::kTimeOut, StatusFlag::kUnknown}));
}

TEST_F(TestClockStatus, FlagsAppendedCorrectly)
{
    ClockStatus<StatusFlag> status{};

    EXPECT_FALSE(status.IsAnyOfFlagsActive(
        {StatusFlag::kSynchToGateway, StatusFlag::kTimeOut, StatusFlag::kSynchronized, StatusFlag::kUnknown}));

    status.AddFlag(StatusFlag::kSynchToGateway);
    status.AddFlag(StatusFlag::kTimeOut);

    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchToGateway));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kTimeOut));
    EXPECT_FALSE(status.IsAnyOfFlagsActive({StatusFlag::kSynchronized, StatusFlag::kUnknown}));

    status.AddFlag(StatusFlag::kUnknown);
    status.AddFlag(StatusFlag::kSynchronized);

    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kTimeOut));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchronized));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchToGateway));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kUnknown));
}

TEST_F(TestClockStatus, FlagAppendedMoreThanOnce)
{
    ClockStatus<StatusFlag> status{StatusFlag::kSynchronized};
    EXPECT_FALSE(status.IsFlagActive(StatusFlag::kSynchToGateway));

    status.AddFlag(StatusFlag::kSynchToGateway);
    status.AddFlag(StatusFlag::kSynchToGateway);

    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchronized));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchToGateway));
    EXPECT_FALSE(status.IsAnyOfFlagsActive({StatusFlag::kTimeOut, StatusFlag::kUnknown}));

    status.AddFlag(StatusFlag::kTimeOut);
    status.AddFlag(StatusFlag::kTimeOut);

    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchronized));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kSynchToGateway));
    EXPECT_TRUE(status.IsFlagActive(StatusFlag::kTimeOut));
    EXPECT_FALSE(status.IsFlagActive(StatusFlag::kUnknown));
}

TEST_F(TestClockStatus, StatusSerializedCorrectlyToFlagsContainer)
{
    ClockStatus<StatusFlag> status{StatusFlag::kSynchToGateway, StatusFlag::kTimeOut};

    const auto raw = status.ToUnderlying();

    ClockStatus<StatusFlag> deserialized{};
    deserialized.FromUnderlying(raw);

    EXPECT_TRUE(deserialized.IsFlagActive(StatusFlag::kSynchToGateway));
    EXPECT_TRUE(deserialized.IsFlagActive(StatusFlag::kTimeOut));
    EXPECT_FALSE(deserialized.IsAnyOfFlagsActive({StatusFlag::kSynchronized, StatusFlag::kUnknown}));

    ClockStatus<StatusFlag> all_set{};
    all_set.FromUnderlying(std::numeric_limits<std::uint32_t>::max());

    EXPECT_TRUE(all_set.IsFlagActive(StatusFlag::kSynchToGateway));
    EXPECT_TRUE(all_set.IsFlagActive(StatusFlag::kTimeOut));
    EXPECT_TRUE(all_set.IsFlagActive(StatusFlag::kSynchronized));
    EXPECT_TRUE(all_set.IsFlagActive(StatusFlag::kUnknown));
}

TEST_F(TestClockStatus, EqualityOperator)
{
    const ClockStatus<StatusFlag> a{StatusFlag::kSynchronized, StatusFlag::kTimeOut};
    const ClockStatus<StatusFlag> b{StatusFlag::kSynchronized, StatusFlag::kTimeOut};
    const ClockStatus<StatusFlag> c{StatusFlag::kSynchToGateway};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST_F(TestClockStatus, DefaultConstructedStatusHasNoFlagsActive)
{
    const ClockStatus<StatusFlag> status{};

    EXPECT_FALSE(status.IsAnyOfFlagsActive(
        {StatusFlag::kTimeOut, StatusFlag::kSynchronized, StatusFlag::kSynchToGateway, StatusFlag::kUnknown}));
    EXPECT_EQ(status.ToUnderlying(), std::uint32_t{0U});
}

TEST_F(TestClockStatus, OutOfRangeFlagAborts)
{
    enum class StatusFlagOutOfRange : std::uint8_t
    {
        kTimeOut        = 0U,
        kSynchronized   = 1U,
        kSynchToGateway = 200U,
        kUnknown        = 255U,
    };

    ClockStatus<StatusFlagOutOfRange> status{};

    status.AddFlag(StatusFlagOutOfRange::kSynchronized);
    EXPECT_TRUE(status.IsFlagActive(StatusFlagOutOfRange::kSynchronized));

    ASSERT_DEATH(status.AddFlag(StatusFlagOutOfRange::kSynchToGateway), "");

    status.AddFlag(StatusFlagOutOfRange::kTimeOut);
    EXPECT_TRUE(status.IsFlagActive(StatusFlagOutOfRange::kTimeOut));

    ASSERT_DEATH(static_cast<void>(status.IsFlagActive(StatusFlagOutOfRange::kUnknown)), "");
}

}  // namespace time
}  // namespace score
