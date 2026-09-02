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
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

#include <chrono>
#include <functional>
#include <limits>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>

namespace score
{
namespace td
{

namespace
{

PtpTimeInfo MakePtpTimeInfoWithRateDeviation(const double rate_deviation)
{
    PtpTimeInfo info{};
    info.ptp_assumed_time = std::chrono::nanoseconds{1234};
    info.local_time = PtpTimeInfo::ReferenceClock::time_point{std::chrono::nanoseconds{5678}};
    info.rate_deviation = rate_deviation;
    info.status = {true, false, false, false, true};
    info.sync_fup_data = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    info.pdelay_data = {11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U, 20U, 21U, 22U};
    return info;
}

}  // namespace

TEST(PtpStatusTest, EqualsWhenAllFieldsMatch)
{
    const PtpStatus first{true, false, true, false, true};
    const PtpStatus second{true, false, true, false, true};

    EXPECT_TRUE(first == second);
}

TEST(PtpStatusTest, NotEqualsWhenAnyFieldDiffers)
{
    const PtpStatus first{true, false, true, false, true};
    const PtpStatus second{false, false, true, false, true};

    EXPECT_FALSE(first == second);
}

TEST(PtpStatusTest, NotEqualsWhenEachFieldDiffers)
{
    const PtpStatus baseline{true, false, true, false, true};

    const std::vector<std::function<void(PtpStatus&)>> mutations = {
        [](PtpStatus& value) {
            value.is_synchronized = !value.is_synchronized;
        },
        [](PtpStatus& value) {
            value.is_timeout = !value.is_timeout;
        },
        [](PtpStatus& value) {
            value.is_time_jump_future = !value.is_time_jump_future;
        },
        [](PtpStatus& value) {
            value.is_time_jump_past = !value.is_time_jump_past;
        },
        [](PtpStatus& value) {
            value.is_correct = !value.is_correct;
        },
    };

    for (const auto& mutation : mutations)
    {
        auto changed = baseline;
        mutation(changed);
        EXPECT_FALSE(baseline == changed);
    }
}

TEST(SyncFupDataTest, EqualsWhenAllFieldsMatch)
{
    const SyncFupData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    const SyncFupData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};

    EXPECT_TRUE(first == second);
}

TEST(SyncFupDataTest, NotEqualsOperatorReturnsTrueWhenDifferent)
{
    const SyncFupData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    const SyncFupData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 10U};

    EXPECT_TRUE(first != second);
}

TEST(SyncFupDataTest, NotEqualsWhenEachFieldDiffers)
{
    const SyncFupData baseline{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};

    const std::vector<std::function<void(SyncFupData&)>> mutations = {
        [](SyncFupData& value) {
            value.precise_origin_timestamp += 10U;
        },
        [](SyncFupData& value) {
            value.reference_global_timestamp += 10U;
        },
        [](SyncFupData& value) {
            value.reference_local_timestamp += 10U;
        },
        [](SyncFupData& value) {
            value.sync_ingress_timestamp += 10U;
        },
        [](SyncFupData& value) {
            value.correction_field += 10U;
        },
        [](SyncFupData& value) {
            value.sequence_id += 1U;
        },
        [](SyncFupData& value) {
            value.pdelay += 10U;
        },
        [](SyncFupData& value) {
            value.port_number += 1U;
        },
        [](SyncFupData& value) {
            value.clock_identity += 10U;
        },
    };

    for (const auto& mutation : mutations)
    {
        auto changed = baseline;
        mutation(changed);
        EXPECT_TRUE(baseline != changed);
    }
}

TEST(PDelayDataTest, EqualsWhenAllFieldsMatch)
{
    const PDelayData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const PDelayData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};

    EXPECT_TRUE(first == second);
}

TEST(PDelayDataTest, NotEqualsOperatorReturnsTrueWhenDifferent)
{
    const PDelayData first{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const PDelayData second{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 13U};

    EXPECT_TRUE(first != second);
}

TEST(PDelayDataTest, NotEqualsWhenEachFieldDiffers)
{
    const PDelayData baseline{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};

    const std::vector<std::function<void(PDelayData&)>> mutations = {
        [](PDelayData& value) {
            value.request_origin_timestamp += 10U;
        },
        [](PDelayData& value) {
            value.request_receipt_timestamp += 10U;
        },
        [](PDelayData& value) {
            value.response_origin_timestamp += 10U;
        },
        [](PDelayData& value) {
            value.response_receipt_timestamp += 10U;
        },
        [](PDelayData& value) {
            value.reference_global_timestamp += 10U;
        },
        [](PDelayData& value) {
            value.reference_local_timestamp += 10U;
        },
        [](PDelayData& value) {
            value.sequence_id += 1U;
        },
        [](PDelayData& value) {
            value.pdelay += 10U;
        },
        [](PDelayData& value) {
            value.req_port_number += 1U;
        },
        [](PDelayData& value) {
            value.req_clock_identity += 10U;
        },
        [](PDelayData& value) {
            value.resp_port_number += 1U;
        },
        [](PDelayData& value) {
            value.resp_clock_identity += 10U;
        },
    };

    for (const auto& mutation : mutations)
    {
        auto changed = baseline;
        mutation(changed);
        EXPECT_TRUE(baseline != changed);
    }
}

TEST(PtpTimeInfoTest, EqualsUsesToleranceForRateDeviation)
{
    const PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    const PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0 + (std::numeric_limits<double>::epsilon() / 2.0));

    EXPECT_TRUE(first == second);
}

TEST(PtpTimeInfoTest, NotEqualsWhenRateDeviationOutsideTolerance)
{
    const PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    const PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0 + (std::numeric_limits<double>::epsilon() * 2.0));

    EXPECT_TRUE(first != second);
}

TEST(PtpTimeInfoTest, EqualsWhenAllFieldsMatch)
{
    const PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    const PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0);

    EXPECT_TRUE(first == second);
}

TEST(PtpTimeInfoTest, NotEqualsWhenStatusDiffers)
{
    PtpTimeInfo first = MakePtpTimeInfoWithRateDeviation(1.0);
    PtpTimeInfo second = MakePtpTimeInfoWithRateDeviation(1.0);
    second.status.is_timeout = !second.status.is_timeout;

    EXPECT_TRUE(first != second);
}

TEST(PtpTimeInfoTest, NotEqualsWhenEachTopLevelFieldDiffers)
{
    const PtpTimeInfo baseline = MakePtpTimeInfoWithRateDeviation(1.0);

    const std::vector<std::function<void(PtpTimeInfo&)>> mutations = {
        [](PtpTimeInfo& value) {
            value.local_time += std::chrono::nanoseconds{1};
        },
        [](PtpTimeInfo& value) {
            value.ptp_assumed_time += std::chrono::nanoseconds{1};
        },
        [](PtpTimeInfo& value) {
            value.rate_deviation += std::numeric_limits<double>::epsilon() * 4.0;
        },
        [](PtpTimeInfo& value) {
            value.status.is_correct = !value.status.is_correct;
        },
        [](PtpTimeInfo& value) {
            value.sync_fup_data.sequence_id += 1U;
        },
        [](PtpTimeInfo& value) {
            value.pdelay_data.sequence_id += 1U;
        },
    };

    for (const auto& mutation : mutations)
    {
        auto changed = baseline;
        mutation(changed);
        EXPECT_TRUE(baseline != changed);
    }
}

TEST(PrintToTest, WritesReadableRepresentationForAllTypes)
{
    const PtpStatus status{true, false, true, false, true};
    const SyncFupData sync_fup_data{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    const PDelayData pdelay_data{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const PtpTimeInfo info = MakePtpTimeInfoWithRateDeviation(1.0);

    std::ostringstream status_stream;
    PrintTo(status, &status_stream);
    EXPECT_FALSE(status_stream.str().empty());

    std::ostringstream sync_fup_stream;
    PrintTo(sync_fup_data, &sync_fup_stream);
    EXPECT_FALSE(sync_fup_stream.str().empty());

    std::ostringstream pdelay_stream;
    PrintTo(pdelay_data, &pdelay_stream);
    EXPECT_FALSE(pdelay_stream.str().empty());

    std::ostringstream info_stream;
    PrintTo(info, &info_stream);
    EXPECT_FALSE(info_stream.str().empty());
}

}  // namespace td
}  // namespace score
