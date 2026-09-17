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

#include "score/time_slave/src/gptp/platform/qnx/bpf_device_path.h"

#include <gtest/gtest.h>

#include <string_view>

namespace score::ts::details
{
namespace
{

TEST(BpfDevicePathTest, UsesDefaultForNullPath)
{
    EXPECT_EQ(std::string_view{ResolveBpfDevicePath(nullptr)}, "/dev/bpf");
}

TEST(BpfDevicePathTest, UsesDefaultForEmptyPath)
{
    EXPECT_EQ(std::string_view{ResolveBpfDevicePath("")}, "/dev/bpf");
}

TEST(BpfDevicePathTest, UsesConfiguredDevicePathUnchanged)
{
    EXPECT_EQ(std::string_view{ResolveBpfDevicePath("/dev/bpf")}, "/dev/bpf");
    EXPECT_EQ(std::string_view{ResolveBpfDevicePath("/alt/dev/bpf")}, "/alt/dev/bpf");
}

}  // namespace
}  // namespace score::ts::details
