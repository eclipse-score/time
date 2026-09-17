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

#include "score/time_slave/src/common/definitions.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <optional>
#include <string_view>

namespace score::ts::env
{
namespace
{

class GetEnvWithDefaultTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char* original = std::getenv(qnx::kBpfDevicePathEnv);
        if (original != nullptr)
        {
            original_value_ = original;
        }
    }

    void TearDown() override
    {
        if (original_value_.has_value())
        {
            setenv(qnx::kBpfDevicePathEnv, original_value_->c_str(), 1);
        }
        else
        {
            unsetenv(qnx::kBpfDevicePathEnv);
        }
    }

    std::optional<std::string> original_value_;
};

TEST_F(GetEnvWithDefaultTest, ReturnsDefaultWhenEnvVarUnset)
{
    unsetenv(qnx::kBpfDevicePathEnv);
    EXPECT_EQ(std::string_view{GetEnvWithDefault(qnx::kBpfDevicePathEnv, qnx::kBpfDevicePathDefault)},
              qnx::kBpfDevicePathDefault);
}

TEST_F(GetEnvWithDefaultTest, ReturnsDefaultWhenEnvVarEmpty)
{
    setenv(qnx::kBpfDevicePathEnv, "", 1);
    EXPECT_EQ(std::string_view{GetEnvWithDefault(qnx::kBpfDevicePathEnv, qnx::kBpfDevicePathDefault)},
              qnx::kBpfDevicePathDefault);
}

TEST_F(GetEnvWithDefaultTest, ReturnsEnvVarWhenSet)
{
    setenv(qnx::kBpfDevicePathEnv, "/alt/dev/bpf", 1);
    EXPECT_EQ(std::string_view{GetEnvWithDefault(qnx::kBpfDevicePathEnv, qnx::kBpfDevicePathDefault)},
              "/alt/dev/bpf");
}

}  // namespace
}  // namespace score::ts::env
