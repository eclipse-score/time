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
#include "score/time_daemon/src/ipc/core/receiver_impl.h"
#include "score/time_daemon/src/ipc/core/shared_memory_handler.h"
#include "score/time_daemon/src/ipc/core/test_types.h"

#include <gtest/gtest.h>

namespace score
{
namespace td
{

class ReceiverTest : public ::testing::Test
{
  public:
    void SetUp() override {};

    void TearDown() override
    {
        // Clean up shared memory to not affect other tests
        score::memory::shared::SharedMemoryFactory::Remove(shared_memory_path_);
    };

    const std::string shared_memory_path_{"/sh_receiver_data"};
};

TEST_F(ReceiverTest, TestReadAndWrite)
{
    // Initialize ipc handler to set some data
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);
    EXPECT_TRUE(handler.Init());

    // Write some data
    test::FakeTimeInfoIpc input_data = {678, 121};
    EXPECT_NO_THROW(handler.Send(input_data));

    // For given shared memory path create receiver
    auto receiver = ReceiverImpl<test::FakeTimeInfoIpc>(shared_memory_path_);

    // Then initialize receiver
    EXPECT_TRUE(receiver.Init());

    // And expect that read data is equal to simulated
    EXPECT_EQ(receiver.Receive().value(), input_data);
}

TEST_F(ReceiverTest, TestReadWithoutInit)
{
    // For given shared memory path create receiver
    auto receiver = ReceiverImpl<test::FakeTimeInfoIpc>(shared_memory_path_);

    // Initialize ipc handler to set some data
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);
    EXPECT_TRUE(handler.Init());

    // Write some data
    test::FakeTimeInfoIpc input_data = {678, 121};
    EXPECT_NO_THROW(handler.Send(input_data));

    // Without init data shall not be be provided
    EXPECT_FALSE(receiver.Receive().has_value());
}

}  // namespace td
}  // namespace score
