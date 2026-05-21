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
#include "score/time_daemon/src/control_flow_divider/ptp/factory.h"
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>

using namespace std::chrono_literals;

namespace score
{
namespace td
{

class PtpControlFlowDividerFactoryTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PtpControlFlowDividerFactoryTest, CreateInstance)
{
    auto divider = CreatePtpControlFlowDivider("TestDivider", 100ms);

    ASSERT_NE(divider, nullptr);
    EXPECT_EQ(divider->GetName(), "TestDivider");
}

TEST_F(PtpControlFlowDividerFactoryTest, CreateMultipleInstances)
{
    auto divider1 = CreatePtpControlFlowDivider("Divider1", 50ms);
    auto divider2 = CreatePtpControlFlowDivider("Divider2", 100ms);

    ASSERT_NE(divider1, nullptr);
    ASSERT_NE(divider2, nullptr);
    EXPECT_EQ(divider1->GetName(), "Divider1");
    EXPECT_EQ(divider2->GetName(), "Divider2");
}

TEST_F(PtpControlFlowDividerFactoryTest, InitializeCreatedInstance)
{
    auto divider = CreatePtpControlFlowDivider("TestDivider", 100ms);

    ASSERT_NE(divider, nullptr);
    EXPECT_TRUE(divider->Init());
}

}  // namespace td
}  // namespace score
