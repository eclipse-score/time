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
#include "score/time_daemon/src/ipc/svt/publisher/factory.h"
#include "score/time_daemon/src/ipc/svt/receiver/factory.h"

#include <gtest/gtest.h>

namespace score
{
namespace td
{

TEST(FactoryImplTest, TestReadAndWrite)
{
    // Create and initialize publisher and receiver
    auto receiver = CreateSvtReceiver();
    auto publisher = CreateSvtPublisher("svt_publisher");

    EXPECT_TRUE(publisher->Init());
    EXPECT_TRUE(receiver->Init());

    // Write some data
    std::chrono::nanoseconds ptp_assumed_time{321};
    PtpTimeInfo::ReferenceClock::time_point local_time{std::chrono::nanoseconds{654}};

    PtpTimeInfo input_data = {ptp_assumed_time, local_time, 0.0, {}, {}, {}};
    EXPECT_NO_THROW(publisher->OnMessage(input_data));

    // Expect that written data is equal to obtained data
    EXPECT_TRUE(receiver->Receive().has_value());
    EXPECT_EQ(receiver->Receive().value(), input_data);
}

}  // namespace td
}  // namespace score
