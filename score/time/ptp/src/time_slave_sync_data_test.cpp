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
#include "score/time/ptp/src/time_slave_sync_data.h"

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

TEST(TimeSlaveSyncDataTest, PrintToStream)
{
    std::ostringstream os;

    const TimeSlaveSyncData<TestTimebase> sync_data{TestTimebase::Timepoint{123ns},
                                                    TestTimebase::Timepoint{456ns},
                                                    LocalPTPDeviceTimerValue{789ns},
                                                    LocalPTPDeviceTimerValue{123ns},
                                                    45,
                                                    67,
                                                    89ns,
                                                    {12345U, 6789U}};

    PrintTo(sync_data, &os);

    EXPECT_STREQ("[123, 456, 789, 123, 45 / 0x10000, 67, 89, (12345, 6789)]", os.str().c_str());
}

TEST(TimeSlaveSyncDataTest, OperatorToStream)
{
    std::stringstream os;

    const TimeSlaveSyncData<TestTimebase> sync_data{TestTimebase::Timepoint{123ns},
                                                    TestTimebase::Timepoint{456ns},
                                                    LocalPTPDeviceTimerValue{789ns},
                                                    LocalPTPDeviceTimerValue{123ns},
                                                    45,
                                                    67,
                                                    89ns,
                                                    {12345U, 6789U}};

    os << sync_data;

    EXPECT_STREQ("[123, 456, 789, 123, 45 / 0x10000, 67, 89, (12345, 6789)]", os.str().c_str());
}

}  // namespace
}  // namespace time
}  // namespace score
