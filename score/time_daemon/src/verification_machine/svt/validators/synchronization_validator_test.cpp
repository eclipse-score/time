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
#include "score/time_daemon/src/verification_machine/svt/validators/synchronization_validator.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

namespace score
{
namespace td
{

struct TestParams
{
    bool simulated_synchronized_status;
    bool expected_is_synchronized;
};

class SynchronizationValidatorParamTest : public ::testing::TestWithParam<std::vector<TestParams>>
{
  public:
    void SetUp() override {}

    void TearDown() override {}
};

INSTANTIATE_TEST_CASE_P(
    TestIfSynchronizedFlagIsNotCleared,
    SynchronizationValidatorParamTest,
    ::testing::Values(std::vector<TestParams>{
        // simulated_synchronized_status, expected_is_synchronized
        {false, false},
        {true, true},
        {false, true},  // In this step even if simulated Sync is false, expected is that it shall be set as true
    }));

TEST_P(SynchronizationValidatorParamTest, ValidationTest)
{
    SynchronizationValidator validator;

    for (const auto& parameter : GetParam())
    {
        PtpStatus in_status{};
        in_status.is_synchronized = parameter.simulated_synchronized_status;

        std::chrono::nanoseconds cur_ptp_time{0};
        PtpTimeInfo::ReferenceClock::time_point cur_local_time{std::chrono::nanoseconds{0}};
        PtpTimeInfo in_data{cur_ptp_time, cur_local_time, 0, in_status, {}, {}};
        auto actualData = validator.Process(in_data);
        EXPECT_EQ(actualData.status.is_synchronized, parameter.expected_is_synchronized);
    }
}

}  // namespace td
}  // namespace score
