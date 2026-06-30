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

TEST(PDelayMeasurementDataTest, PrintToStream)
{
    std::ostringstream os;

    const PDelayMeasurementData<TestTimebase> pdelay_data{LocalPTPDeviceTimerValue{12ns},
                                                          MasterPTPDeviceTimerValue{34ns},
                                                          MasterPTPDeviceTimerValue{56ns},
                                                          LocalPTPDeviceTimerValue{78ns},
                                                          TestTimebase::Timepoint{90ns},
                                                          LocalPTPDeviceTimerValue{123ns},
                                                          456U,
                                                          789ns,
                                                          {123U, 45U},
                                                          {678U, 90U}};

    PrintTo(pdelay_data, &os);

    EXPECT_STREQ("[12, 34, 56, 78, 90, 123, 456, 789, (123, 45), (678, 90)]", os.str().c_str());
}

TEST(PDelayMeasurementDataTest, OperatorToStream)
{
    std::stringstream os;

    const PDelayMeasurementData<TestTimebase> pdelay_data{LocalPTPDeviceTimerValue{12ns},
                                                          MasterPTPDeviceTimerValue{34ns},
                                                          MasterPTPDeviceTimerValue{56ns},
                                                          LocalPTPDeviceTimerValue{78ns},
                                                          TestTimebase::Timepoint{90ns},
                                                          LocalPTPDeviceTimerValue{123ns},
                                                          456U,
                                                          789ns,
                                                          {123U, 45U},
                                                          {678U, 90U}};

    os << pdelay_data;

    EXPECT_STREQ("[12, 34, 56, 78, 90, 123, 456, 789, (123, 45), (678, 90)]", os.str().c_str());
}

}  // namespace
}  // namespace time
}  // namespace score
