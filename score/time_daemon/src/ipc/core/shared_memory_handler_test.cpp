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
#include "score/time_daemon/src/ipc/core/shared_memory_handler.h"
#include "score/time_daemon/src/ipc/core/test_types.h"

#include <gtest/gtest.h>

namespace score
{
namespace td
{

class SharedMemoryHandlerTest : public ::testing::Test
{
  public:
    void SetUp() override {};

    void TearDown() override
    {
        // Clean up shared memory to not affect other tests
        score::memory::shared::SharedMemoryFactory::Remove(shared_memory_path_);
    };

    const std::string shared_memory_path_{"/sh_some_data"};
};

TEST_F(SharedMemoryHandlerTest, TestReadAndWrite)
{

    // For given shared memory path create handler
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);

    // Then initialize and map shm resource
    EXPECT_TRUE(handler.Init());

    // Write some data
    test::FakeTimeInfoIpc input_data = {123, 456};
    EXPECT_NO_THROW(handler.Send(input_data));

    // Then expect that obtained data is equal to simulated.
    auto data = handler.Receive();
    EXPECT_TRUE(data.has_value());
    EXPECT_EQ(data.value(), input_data);
}

TEST_F(SharedMemoryHandlerTest, TestWriteWithoutInit)
{
    // For given shared memory path create handler
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);

    // Write some data
    test::FakeTimeInfoIpc input_data = {123, 456};
    EXPECT_NO_THROW(handler.Send(input_data));

    // Then expect that obtained data is not equal to simulated
    auto data = handler.Receive();
    EXPECT_FALSE(data.has_value());
}

}  // namespace td
}  // namespace score
