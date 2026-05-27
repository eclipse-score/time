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
#include "score/time_daemon/src/application/svt/svt_handler.h"
#include "score/time_daemon/src/ipc/core/shared_memory_handler.h"
#include "score/time_daemon/src/ipc/svt/receiver/factory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>

namespace score
{
namespace td
{

using ::testing::_;
using ::testing::DoAll;
using ::testing::Exactly;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;

class SvtHandlerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mocks::LibqgptpMock::CreateMockInstance();
    }

    void TearDown() override
    {
        mocks::LibqgptpMock::DestroyMockInstance();
    }

    void CheckTimeSync(SvtHandler& handler,
                       std::shared_ptr<SvtReceiver> receiver,
                       uint64_t expected_time,
                       int kRetryThreshold = 100)
    {
        score::cpp::stop_token stop_token{};
        std::uint64_t obtained_time{0U};
        bool is_synchronized{false};

        for (int i = 0; i < kRetryThreshold; i++)
        {
            // Mock current PTP time
            EXPECT_CALL(mocks::LibqgptpMock::GetMockInstance(), gptpGetCurPtpTime(_))
                .WillRepeatedly(DoAll(SetArgPointee<0>(expected_time), Return(true)));

            // Mock GPTP status
            const helper_GPTPStatsType_t expected_qgptp_time_base_Status{GPTP_STATUS_SYNCHRONIZED_H, 0.0, false, 0};
            EXPECT_CALL(mocks::LibqgptpMock::GetMockInstance(), getgPTPStatus(_))
                .WillRepeatedly(DoAll(SetArgPointee<0>(expected_qgptp_time_base_Status), Return(STAT_OK)));

            // Mock sync measurement
            helper_syncMesaurementData_t sync_measurement_data{};
            sync_measurement_data.sequence_id = i;
            EXPECT_CALL(mocks::LibqgptpMock::GetMockInstance(), gptpGetSyncMeasurementData(_))
                .WillRepeatedly(DoAll(SetArgPointee<0>(sync_measurement_data), Return(STAT_OK)));

            // Mock pDelay measurement
            helper_pDelayMeasurementData_t pdelay_measurement_data{};
            EXPECT_CALL(mocks::LibqgptpMock::GetMockInstance(), gptpGetPDelayMeasurementData(_))
                .WillRepeatedly(DoAll(SetArgPointee<0>(pdelay_measurement_data), Return(STAT_OK)));

            // Run the handler once
            ASSERT_NO_THROW(handler.RunOnce(stop_token));

            // Small sleep to allow async propagation
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto data = receiver->Receive();
            EXPECT_TRUE(data.has_value());

            obtained_time = data.value().ptp_assumed_time;
            is_synchronized = data.value().status.is_synchronized;

            if (obtained_time == expected_time)
                break;
        }

        EXPECT_EQ(obtained_time, expected_time);
        EXPECT_TRUE(is_synchronized);
    }
};

TEST_F(SvtHandlerTest, CreateAndDestroy)
{
    // score::memory::shared::SharedMemoryFactory::Remove("/svt_shmem_path");

    auto handler = std::make_unique<SvtHandler>();
    ASSERT_TRUE(handler != nullptr);

    EXPECT_CALL(mocks::LibqgptpMock::GetMockInstance(), gptpInit()).Times(Exactly(1)).WillOnce(Return(true));
    ASSERT_NO_THROW(handler->Initialize());

    // Prepare the receiver to read published data
    auto receiver = CreateSvtReceiver();
    // Initialization shall pass
    EXPECT_TRUE(receiver->Init());

    std::vector<uint64_t> expected_times = {1234567U,
                                            18471583703745U,
                                            9876543210U,
                                            5555555U,
                                            999999999U,
                                            123123123U,
                                            7777777U,
                                            3141592653U,
                                            2718281828U,
                                            1618033988U};

    for (const auto& expected_time : expected_times)
    {
        CheckTimeSync(*handler, receiver, expected_time);
    }

    ASSERT_NO_THROW(handler->Stop());
}

}  // namespace td
}  // namespace score
