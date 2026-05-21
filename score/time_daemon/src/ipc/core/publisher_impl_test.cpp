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
#include "score/time_daemon/src/ipc/core/publisher_impl.h"
#include "score/time_daemon/src/ipc/core/shared_memory_handler.h"
#include "score/time_daemon/src/ipc/core/test_types.h"

#include <gtest/gtest.h>

namespace score
{
namespace td
{

class PublisherTest : public ::testing::Test
{
  public:
    void SetUp() override {};

    void TearDown() override
    {
        // Clean up shared memory to not affect other tests
        score::memory::shared::SharedMemoryFactory::Remove(shared_memory_path_);
    };

    const std::string shared_memory_path_{"/sh_publisher_data"};
};

TEST_F(PublisherTest, TestReadAndWrite)
{
    // For given shared memory path create publisher
    auto publisher = PublisherImpl<test::FakeTimeInfo, test::FakeTimeInfoIpc>("some_machine", shared_memory_path_);

    // Then initialize publisher
    EXPECT_TRUE(publisher.Init());

    // Write some data
    test::FakeTimeInfo input_data = {123, 456};
    EXPECT_NO_THROW(publisher.OnMessage(input_data));

    // Then just initialize ipc handler to verify the required data
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);
    EXPECT_TRUE(handler.Init());
    EXPECT_EQ(handler.Receive().value(), input_data);
}

TEST_F(PublisherTest, TestWriteWithoutInit)
{
    // For given shared memory path create publisher
    auto publisher = PublisherImpl<test::FakeTimeInfo, test::FakeTimeInfoIpc>("some_machine", shared_memory_path_);

    // Write some data
    test::FakeTimeInfo input_data = {234, 531};
    EXPECT_NO_THROW(publisher.OnMessage(input_data));

    // Then just initialize ipc handler to verify the received data does not matched to updated
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);
    EXPECT_TRUE(handler.Init());

    auto data = handler.Receive();

    // Without init data shall not be updated
    EXPECT_TRUE(data.has_value());
    EXPECT_NE(data.value(), input_data);
}

}  // namespace td
}  // namespace score
