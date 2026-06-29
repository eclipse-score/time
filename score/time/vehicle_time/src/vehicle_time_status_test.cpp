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
#include "score/time/vehicle_time/src/vehicle_time.h"

#include <gtest/gtest.h>

#include <sstream>

namespace score
{
namespace time
{
namespace
{

using Flag = VehicleTime::StatusFlag;

class TestVehicleTimeStatus : public ::testing::Test
{
};

TEST_F(TestVehicleTimeStatus, IsReliableReturnsTrueOnlyWhenFlagSetWithoutErrorFlags)
{
    VehicleTimeStatus status{};
    EXPECT_FALSE(status.IsReliable());

    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.IsReliable());

    status.flags.AddFlag(Flag::kTimeOut);
    EXPECT_FALSE(status.IsReliable());
}

TEST_F(TestVehicleTimeStatus, IsReliableReturnsFalseWhenTimeLeapFutureSet)
{
    VehicleTimeStatus status{};
    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.IsReliable());

    status.flags.AddFlag(Flag::kTimeLeapFuture);
    EXPECT_FALSE(status.IsReliable());
}

TEST_F(TestVehicleTimeStatus, IsReliableReturnsFalseWhenTimeLeapPastSet)
{
    VehicleTimeStatus status{};
    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.IsReliable());

    status.flags.AddFlag(Flag::kTimeLeapPast);
    EXPECT_FALSE(status.IsReliable());
}

TEST_F(TestVehicleTimeStatus, HasBeenSynchronizedReturnsTrueWhenSynchronizedFlagIsSet)
{
    VehicleTimeStatus status{};
    EXPECT_FALSE(status.HasBeenSynchronized());

    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.HasBeenSynchronized());
}

TEST_F(TestVehicleTimeStatus, HasBeenSynchronizedReturnsTrueEvenWithFaultFlags)
{
    VehicleTimeStatus status{};
    status.flags.AddFlag(Flag::kSynchronized);
    status.flags.AddFlag(Flag::kTimeOut);
    EXPECT_TRUE(status.HasBeenSynchronized());
    EXPECT_FALSE(status.IsReliable());
}

TEST_F(TestVehicleTimeStatus, IsConsistentReturnsFalseWhenNoFlagsSet)
{
    const VehicleTimeStatus status{};
    EXPECT_FALSE(status.IsConsistent());
}

TEST_F(TestVehicleTimeStatus, IsConsistentReturnsTrueWithSingleFlagSet)
{
    VehicleTimeStatus status{};
    status.flags.AddFlag(Flag::kTimeOut);
    EXPECT_TRUE(status.IsConsistent());

    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.IsConsistent());
}

TEST_F(TestVehicleTimeStatus, IsConsistentReturnsTrueWithLeapPastFlag)
{
    VehicleTimeStatus status{};
    status.flags.AddFlag(Flag::kTimeOut);
    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.IsConsistent());

    status.flags.AddFlag(Flag::kTimeLeapPast);
    EXPECT_TRUE(status.IsConsistent());
}

TEST_F(TestVehicleTimeStatus, IsConsistentReturnsTrueWithLeapFutureFlag)
{
    VehicleTimeStatus status{};
    status.flags.AddFlag(Flag::kTimeOut);
    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.IsConsistent());

    status.flags.AddFlag(Flag::kTimeLeapFuture);
    EXPECT_TRUE(status.IsConsistent());
}

TEST_F(TestVehicleTimeStatus, IsConsistentReturnsFalseWhenBothLeapFlagsSet)
{
    VehicleTimeStatus status{};
    status.flags.AddFlag(Flag::kTimeOut);
    status.flags.AddFlag(Flag::kSynchronized);
    status.flags.AddFlag(Flag::kTimeLeapPast);
    EXPECT_TRUE(status.IsConsistent());

    status.flags.AddFlag(Flag::kTimeLeapFuture);
    EXPECT_FALSE(status.IsConsistent());
}

TEST_F(TestVehicleTimeStatus, PrintToFormatsActiveFlagsCorrectly)
{
    ClockStatus<Flag> status{Flag::kTimeOut};

    std::ostringstream os;
    os << status;

    EXPECT_STREQ(
        "[kTimeOut: true, kSynchronized: false, kTimeLeapFuture: false, "
        "kTimeLeapPast: false, ]",
        os.str().c_str());
}

TEST_F(TestVehicleTimeStatus, IsFlagActiveDelegatesCorrectly)
{
    VehicleTimeStatus status{};
    EXPECT_FALSE(status.IsFlagActive(Flag::kSynchronized));

    status.flags.AddFlag(Flag::kSynchronized);
    EXPECT_TRUE(status.IsFlagActive(Flag::kSynchronized));
    EXPECT_FALSE(status.IsFlagActive(Flag::kTimeOut));
}

}  // namespace
}  // namespace time
}  // namespace score
