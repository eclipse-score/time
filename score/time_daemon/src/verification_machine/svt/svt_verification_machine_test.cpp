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
#include "score/time_daemon/src/verification_machine/svt/svt_verification_machine.h"
#include "score/time_daemon/src/verification_machine/svt/factory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace score
{
namespace td
{

class SvtVerificationMachineTest : public ::testing::Test
{
  protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(SvtVerificationMachineTest, ProcessesValidTimeInfo)
{
    std::shared_ptr<SvtVerificationMachine> vm = CreateSvtVerificationMachine("TestMachine");

    PtpTimeInfo::ReferenceClock::time_point cur_local_time{std::chrono::nanoseconds{2000000000ULL}};
    std::chrono::nanoseconds cur_ptp_time{1000000000ULL};
    const PtpTimeInfo test_data = {cur_ptp_time, cur_local_time, 0.0, {}, {}, {}};

    EXPECT_NO_THROW(vm->OnMessage(test_data));
}

TEST_F(SvtVerificationMachineTest, HandlesPipelineValidation)
{
    std::shared_ptr<SvtVerificationMachine> vm = CreateSvtVerificationMachine("TestMachine");

    PtpTimeInfo::ReferenceClock::time_point cur_local_time{std::chrono::nanoseconds{2000000000ULL}};
    std::chrono::nanoseconds cur_ptp_time{1000000000ULL};
    const PtpTimeInfo test_data = {cur_ptp_time, cur_local_time, 0.0, {}, {}, {}};

    // Mock the publish callback to verify output
    bool publish_called = false;
    PtpTimeInfo published_data;

    vm->SetPublishCallback([&](const PtpTimeInfo& data) {
        publish_called = true;
        published_data = data;
    });

    vm->OnMessage(test_data);

    EXPECT_TRUE(publish_called);
    EXPECT_EQ(published_data, test_data);
}

}  // namespace td
}  // namespace score
