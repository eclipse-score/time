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
constexpr ::timespec kPtpTs{20, 2};
constexpr ::timespec kMonoTs{30, 3};

bool TsEq(const ::timespec& a, const ::timespec& b) noexcept
{
    return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}

TEST(TimestampSelectTest, SoftwareLocked_IgnoresValidHardwareInputs_ReturnsMonoFallback)
{
    // Even though both BPF and PTP_GET_TIME report valid timestamps, a software-locked
    // interface must never mix in a hardware reading — this is the AC3 guarantee.
    const ::timespec result = SelectTimestamp(
        /*hw_ts_supported=*/false, /*bpf_ts_valid=*/true, kBpfTs, /*ptp_get_time_valid=*/true, kPtpTs, kMonoTs);
    EXPECT_TRUE(TsEq(result, kMonoTs));
}

TEST(TimestampSelectTest, SoftwareLocked_NoHardwareInputs_ReturnsMonoFallback)
{
    const ::timespec result =
        SelectTimestamp(/*hw_ts_supported=*/false, false, ::timespec{}, false, ::timespec{}, kMonoTs);
    EXPECT_TRUE(TsEq(result, kMonoTs));
}

TEST(TimestampSelectTest, HardwareLocked_BpfValid_PrefersBpfOverPtpGetTime)
{
    const ::timespec result = SelectTimestamp(
        /*hw_ts_supported=*/true, /*bpf_ts_valid=*/true, kBpfTs, /*ptp_get_time_valid=*/true, kPtpTs, kMonoTs);
    EXPECT_TRUE(TsEq(result, kBpfTs));
}

TEST(TimestampSelectTest, HardwareLocked_BpfInvalid_FallsBackToPtpGetTime)
{
    const ::timespec result = SelectTimestamp(
        /*hw_ts_supported=*/true, /*bpf_ts_valid=*/false, ::timespec{}, /*ptp_get_time_valid=*/true, kPtpTs, kMonoTs);
    EXPECT_TRUE(TsEq(result, kPtpTs));
}

TEST(TimestampSelectTest, HardwareLocked_NeitherBpfNorPtpGetTimeValid_ReturnsSentinel_NeverMono)
{
    const ::timespec result = SelectTimestamp(
        /*hw_ts_supported=*/true,
        /*bpf_ts_valid=*/false,
        ::timespec{},
        /*ptp_get_time_valid=*/false,
        ::timespec{},
        kMonoTs);
    EXPECT_TRUE(TsEq(result, kNoTimestamp));
}

}  // namespace
}  // namespace score::ts::details::qnx
