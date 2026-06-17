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
    EXPECT_TRUE(pub_.Open(UniqueShmName()));
}

TEST_F(GptpIpcPublisherTest, Publish_WithoutInit_DoesNotCrash)
{
    score::ts::GptpIpcData data{};
    EXPECT_NO_THROW(pub_.Publish(data));
}

TEST_F(GptpIpcPublisherTest, Close_CalledTwice_DoesNotCrash)
{
    ASSERT_TRUE(pub_.Open(UniqueShmName()));
    pub_.Close();
    EXPECT_NO_THROW(pub_.Close());
}

TEST_F(GptpIpcPublisherTest, Close_WithoutOpen_DoesNotCrash)
{
    EXPECT_NO_THROW(pub_.Close());
}

TEST_F(GptpIpcPublisherTest, Open_CalledTwice_ReturnsTrueOnSecondCall)
{
    // region_ != nullptr after first Init → second call returns true immediately.
    ASSERT_TRUE(pub_.Open(UniqueShmName()));
    EXPECT_TRUE(pub_.Open(UniqueShmName()));
}

}  // namespace details
}  // namespace ts
}  // namespace score
