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
#include "score/time/ptp/src/pdelay_measurement_data.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <sstream>

namespace score
{
namespace time
{
namespace
{

using namespace std::chrono_literals;

// Minimal synthetic timebase — ptp_types are timebase-agnostic; only Timepoint is needed.
struct TestTimebase
{
    using Timepoint = std::chrono::time_point<TestTimebase, std::chrono::nanoseconds>;
};

PDelayMeasurementData<TestTimebase> MakePDelayData()
{
    return PDelayMeasurementData<TestTimebase>{LocalPTPDeviceTimerValue{12ns},
                                               MasterPTPDeviceTimerValue{34ns},
                                               MasterPTPDeviceTimerValue{56ns},
                                               LocalPTPDeviceTimerValue{78ns},
                                               TestTimebase::Timepoint{90ns},
                                               LocalPTPDeviceTimerValue{123ns},
                                               456U,
                                               789ns,
                                               {123U, 45U},
                                               {678U, 90U}};
}

TEST(PDelayMeasurementDataTest, PrintToStream)
{
    std::ostringstream os;

    const PDelayMeasurementData<TestTimebase> pdelay_data{MakePDelayData()};

    PrintTo(pdelay_data, &os);

    EXPECT_STREQ("[12, 34, 56, 78, 90, 123, 456, 789, (123, 45), (678, 90)]", os.str().c_str());
}

TEST(PDelayMeasurementDataTest, OperatorToStream)
{
    std::stringstream os;

    const PDelayMeasurementData<TestTimebase> pdelay_data{MakePDelayData()};

    os << pdelay_data;

    EXPECT_STREQ("[12, 34, 56, 78, 90, 123, 456, 789, (123, 45), (678, 90)]", os.str().c_str());
}

TEST(PDelayMeasurementDataTest, EqualsWhenAllFieldsMatch)
{
    const PDelayMeasurementData<TestTimebase> first{MakePDelayData()};
    const PDelayMeasurementData<TestTimebase> second{MakePDelayData()};

    EXPECT_TRUE(first == second);
    EXPECT_FALSE(first != second);
}

TEST(PDelayMeasurementDataTest, NotEqualsWhenAnyFieldDiffers)
{
    const PDelayMeasurementData<TestTimebase> baseline{MakePDelayData()};

    const auto expect_not_equal = [&baseline](const auto mutate) {
        auto changed = baseline;
        mutate(changed);
        EXPECT_FALSE(baseline == changed);
        EXPECT_TRUE(baseline != changed);
    };

    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.request_origin_timestamp += 1ns;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.request_receipt_timestamp += 1ns;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.response_origin_timestamp += 1ns;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.response_receipt_timestamp += 1ns;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.reference_global_timestamp += 1ns;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.reference_local_timestamp += 1ns;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.sequence_id = static_cast<std::uint16_t>(value.sequence_id + 1U);
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.pdelay += 1ns;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.request_port_identity.clock_identity += 1U;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.request_port_identity.port_number =
            static_cast<std::uint16_t>(value.request_port_identity.port_number + 1U);
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.response_port_identity.clock_identity += 1U;
    });
    expect_not_equal([](PDelayMeasurementData<TestTimebase>& value) {
        value.response_port_identity.port_number =
            static_cast<std::uint16_t>(value.response_port_identity.port_number + 1U);
    });
}

}  // namespace
}  // namespace time
}  // namespace score
