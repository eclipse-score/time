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

TimeSlaveSyncData<TestTimebase> MakeSyncData()
{
    return TimeSlaveSyncData<TestTimebase>{TestTimebase::Timepoint{123ns},
                                           TestTimebase::Timepoint{456ns},
                                           LocalPTPDeviceTimerValue{789ns},
                                           LocalPTPDeviceTimerValue{123ns},
                                           45,
                                           67,
                                           89ns,
                                           {12345U, 6789U}};
}

TEST(TimeSlaveSyncDataTest, PrintToStream)
{
    std::ostringstream os;

    const TimeSlaveSyncData<TestTimebase> sync_data{MakeSyncData()};

    PrintTo(sync_data, &os);

    EXPECT_STREQ("[123, 456, 789, 123, 45 / 0x10000, 67, 89, (12345, 6789)]", os.str().c_str());
}

TEST(TimeSlaveSyncDataTest, OperatorToStream)
{
    std::stringstream os;

    const TimeSlaveSyncData<TestTimebase> sync_data{MakeSyncData()};

    os << sync_data;

    EXPECT_STREQ("[123, 456, 789, 123, 45 / 0x10000, 67, 89, (12345, 6789)]", os.str().c_str());
}

TEST(PortIdentityTest, EqualsWhenAllFieldsMatch)
{
    const PortIdentity first{12345U, 6789U};
    const PortIdentity second{12345U, 6789U};

    EXPECT_TRUE(first == second);
    EXPECT_FALSE(first != second);
}

TEST(PortIdentityTest, NotEqualsWhenClockIdentityDiffers)
{
    const PortIdentity first{12345U, 6789U};
    const PortIdentity second{12346U, 6789U};

    EXPECT_FALSE(first == second);
    EXPECT_TRUE(first != second);
}

TEST(PortIdentityTest, NotEqualsWhenPortNumberDiffers)
{
    const PortIdentity first{12345U, 6789U};
    const PortIdentity second{12345U, 6790U};

    EXPECT_FALSE(first == second);
    EXPECT_TRUE(first != second);
}

TEST(TimeSlaveSyncDataTest, EqualsWhenAllFieldsMatch)
{
    const TimeSlaveSyncData<TestTimebase> first{MakeSyncData()};
    const TimeSlaveSyncData<TestTimebase> second{MakeSyncData()};

    EXPECT_TRUE(first == second);
    EXPECT_FALSE(first != second);
}

TEST(TimeSlaveSyncDataTest, NotEqualsWhenAnyFieldDiffers)
{
    const TimeSlaveSyncData<TestTimebase> baseline{MakeSyncData()};

    const auto expect_not_equal = [&baseline](const auto mutate) {
        auto changed = baseline;
        mutate(changed);
        EXPECT_FALSE(baseline == changed);
        EXPECT_TRUE(baseline != changed);
    };

    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.precise_origin_timestamp += 1ns;
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.reference_global_timestamp += 1ns;
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.reference_local_timestamp += 1ns;
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.sync_ingress_timestamp += 1ns;
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.correction_field += 1;
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.sequence_id = static_cast<std::uint16_t>(value.sequence_id + 1U);
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.pdelay += 1ns;
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.source_port_identity.clock_identity += 1U;
    });
    expect_not_equal([](TimeSlaveSyncData<TestTimebase>& value) {
        value.source_port_identity.port_number =
            static_cast<std::uint16_t>(value.source_port_identity.port_number + 1U);
    });
}

}  // namespace
}  // namespace time
}  // namespace score
