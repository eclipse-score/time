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
#include "score/ts_client/src/gptp_ipc_receiver.h"
#include "score/ts_client/src/gptp_ipc_publisher.h"
#include "score/ts_client/src/gptp_ipc_test_utils.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace score
{
namespace ts
{
namespace details
{

class GptpIpcReceiverTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        rx_.Close();
    }

    GptpIpcReceiver rx_;
};

TEST_F(GptpIpcReceiverTest, Open_ShmNotExist_ReturnsFalse)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__receiver_multi_reader");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty("Description", "Receiver::Open with non-existent shared memory returns false.");

    EXPECT_FALSE(rx_.Open("/gptp_nonexistent_" + std::to_string(::getpid())));
}

TEST_F(GptpIpcReceiverTest, Close_WithoutInit_DoesNotCrash)
{
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__shared_memory_mgmt");
    ::testing::Test::RecordProperty("TestType", "fault-injection");
    ::testing::Test::RecordProperty("DerivationTechnique", "error-guessing");
    ::testing::Test::RecordProperty("Description",
                                    "Receiver::Close without prior Open does not crash (defensive behavior).");

    EXPECT_NO_THROW(rx_.Close());
}

TEST_F(GptpIpcReceiverTest, Close_CalledTwice_DoesNotCrash)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__shared_memory_mgmt");
    ::testing::Test::RecordProperty("TestType", "interface-test");
    ::testing::Test::RecordProperty("DerivationTechnique", "boundary-values");
    ::testing::Test::RecordProperty("Description",
                                    "Receiver::Close called twice does not crash (idempotent behavior).");

    EXPECT_NO_THROW(rx_.Close());
    EXPECT_NO_THROW(rx_.Close());
}

TEST_F(GptpIpcReceiverTest, Receive_WithoutInit_ReturnsNullopt)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__data_validity");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty("Description", "Receiver::Receive without prior Open returns empty optional.");

    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcReceiverTest, Open_CalledTwice_ReturnsTrueOnSecondCall)
{
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__receiver_multi_reader");
    ::testing::Test::RecordProperty("TestType", "interface-test");
    ::testing::Test::RecordProperty("DerivationTechnique", "boundary-values");
    ::testing::Test::RecordProperty("Description",
                                    "Receiver::Open called twice returns true on second call (idempotent behavior).");

    // region_ != nullptr after first Open → second call returns true immediately.
    GptpIpcPublisher pub;
    const std::string name = UniqueShmName();
    ASSERT_TRUE(pub.Open(name));
    ASSERT_TRUE(rx_.Open(name));
    EXPECT_TRUE(rx_.Open(name));
    pub.Close();
}

TEST_F(GptpIpcReceiverTest, Open_TooSmallShm_ReturnsFalse)
{
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__shm_validation");
    ::testing::Test::RecordProperty("TestType", "fault-injection");
    ::testing::Test::RecordProperty("DerivationTechnique", "error-guessing");
    ::testing::Test::RecordProperty("Description",
                                    "Receiver::Open with undersized shared memory region returns false.");

    // Create a shm segment smaller than GptpIpcRegion so the fstat size check fails.
    const std::string name = UniqueShmName();
    const int fd = ::shm_open(name.c_str(), O_CREAT | O_RDWR, 0600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(::ftruncate(fd, 1), 0);
    ::close(fd);

    EXPECT_FALSE(rx_.Open(name));

    ::shm_unlink(name.c_str());
}

}  // namespace details
}  // namespace ts
}  // namespace score
