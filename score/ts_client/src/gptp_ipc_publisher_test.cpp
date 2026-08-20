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
#include "score/ts_client/src/gptp_ipc_publisher.h"
#include "score/ts_client/src/gptp_ipc_test_utils.h"

#include <gtest/gtest.h>

namespace score
{
namespace ts
{
namespace details
{

class GptpIpcPublisherTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        pub_.Close();
    }

    GptpIpcPublisher pub_;
};

TEST_F(GptpIpcPublisherTest, Open_ValidName_ReturnsTrue)
{
    ::testing::Test::RecordProperty("FullyVerifies",
                                    "comp_req__ts_client__shared_memory_mgmt,comp_req__ts_client__publisher_creates");
    ::testing::Test::RecordProperty("TestType", "requirements-based");
    ::testing::Test::RecordProperty("DerivationTechnique", "equivalence-classes");
    ::testing::Test::RecordProperty(
        "Description", "Publisher::Open with valid shared memory name creates shared memory segment and returns true.");

    EXPECT_TRUE(pub_.Open(UniqueShmName()));
}

TEST_F(GptpIpcPublisherTest, Publish_WithoutInit_DoesNotCrash)
{
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__seqlock_protocol");
    ::testing::Test::RecordProperty("TestType", "fault-injection");
    ::testing::Test::RecordProperty("DerivationTechnique", "error-guessing");
    ::testing::Test::RecordProperty("Description",
                                    "Publisher::Publish without prior Open does not crash (defensive behavior).");

    score::ts::GptpIpcData data{};
    EXPECT_NO_THROW(pub_.Publish(data));
}

TEST_F(GptpIpcPublisherTest, Close_CalledTwice_DoesNotCrash)
{
    ::testing::Test::RecordProperty("FullyVerifies", "comp_req__ts_client__shared_memory_mgmt");
    ::testing::Test::RecordProperty("TestType", "interface-test");
    ::testing::Test::RecordProperty("DerivationTechnique", "boundary-values");
    ::testing::Test::RecordProperty("Description",
                                    "Publisher::Close called twice does not crash (idempotent behavior).");

    ASSERT_TRUE(pub_.Open(UniqueShmName()));
    pub_.Close();
    EXPECT_NO_THROW(pub_.Close());
}

TEST_F(GptpIpcPublisherTest, Close_WithoutOpen_DoesNotCrash)
{
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__shared_memory_mgmt");
    ::testing::Test::RecordProperty("TestType", "fault-injection");
    ::testing::Test::RecordProperty("DerivationTechnique", "error-guessing");
    ::testing::Test::RecordProperty("Description",
                                    "Publisher::Close without prior Open does not crash (defensive behavior).");

    EXPECT_NO_THROW(pub_.Close());
}

TEST_F(GptpIpcPublisherTest, Open_CalledTwice_ReturnsTrueOnSecondCall)
{
    ::testing::Test::RecordProperty("PartiallyVerifies", "comp_req__ts_client__shared_memory_mgmt");
    ::testing::Test::RecordProperty("TestType", "interface-test");
    ::testing::Test::RecordProperty("DerivationTechnique", "boundary-values");
    ::testing::Test::RecordProperty("Description",
                                    "Publisher::Open called twice returns true on second call (idempotent behavior).");

    // region_ != nullptr after first Init → second call returns true immediately.
    ASSERT_TRUE(pub_.Open(UniqueShmName()));
    EXPECT_TRUE(pub_.Open(UniqueShmName()));
}

}  // namespace details
}  // namespace ts
}  // namespace score
