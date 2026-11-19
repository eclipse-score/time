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
#include "score/time_slave/src/gptp/platform/qnx/timestamp_select.h"

#include <gtest/gtest.h>

namespace score::ts::details::qnx
{
namespace
{

constexpr ::timespec kBpfTs{10, 1};
constexpr ::timespec kMonoTs{30, 3};

bool TsEq(const ::timespec& a, const ::timespec& b) noexcept
{
    return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}

TEST(TimestampSelectTest, SoftwareLocked_IgnoresValidHardwareInput_ReturnsMonoFallback)
{
    // A software-locked interface must never mix in a hardware reading — this is the AC3 guarantee.
    const ::timespec result = SelectTimestamp(/*hw_ts_supported=*/false, /*bpf_ts_valid=*/true, kBpfTs, kMonoTs);
    EXPECT_TRUE(TsEq(result, kMonoTs));
}

TEST(TimestampSelectTest, SoftwareLocked_NoHardwareInput_ReturnsMonoFallback)
{
    const ::timespec result = SelectTimestamp(/*hw_ts_supported=*/false, false, ::timespec{}, kMonoTs);
    EXPECT_TRUE(TsEq(result, kMonoTs));
}

TEST(TimestampSelectTest, HardwareLocked_BpfValid_ReturnsBpf)
{
    const ::timespec result = SelectTimestamp(/*hw_ts_supported=*/true, /*bpf_ts_valid=*/true, kBpfTs, kMonoTs);
    EXPECT_TRUE(TsEq(result, kBpfTs));
}

TEST(TimestampSelectTest, HardwareLocked_BpfInvalid_ReturnsSentinel_NeverMono)
{
    const ::timespec result = SelectTimestamp(/*hw_ts_supported=*/true, false, ::timespec{}, kMonoTs);
    EXPECT_TRUE(TsEq(result, kNoTimestamp));
}

TEST(SelectHwTsSupportedTest, AllCapabilitiesPresent_LocksHardware)
{
    EXPECT_TRUE(SelectHwTsSupported(true, true, true));
}

TEST(SelectHwTsSupportedTest, VirtioLike_TimestampIoctlsOkButNoPtpGetTime_LocksSoftware)
{
    EXPECT_FALSE(SelectHwTsSupported(true, true, false));
}

TEST(SelectHwTsSupportedTest, MissingRxOrTxTimestamping_LocksSoftware)
{
    EXPECT_FALSE(SelectHwTsSupported(false, true, true));
    EXPECT_FALSE(SelectHwTsSupported(true, false, true));
}

}  // namespace
}  // namespace score::ts::details::qnx
