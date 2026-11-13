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
#include "score/time_daemon/src/application/timebase_handlers/score/vehicle_time/score_vehicle_timebase_handler.h"
#include "score/time_daemon/src/ipc/svt/receiver/factory.h"
#include "score/ts_client/src/gptp_ipc_publisher.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <thread>

namespace score
{
namespace td
{

class VehicleTimeHandlerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ASSERT_TRUE(publisher_.Open());
        publisher_.Publish(score::ts::GptpIpcData{});
        handler_.Initialize();
        handler_.Start(stop_source_.get_token());
    }

    void TearDown() override
    {
        stop_source_.request_stop();
        handler_.Stop();
    }

    void CheckTimeSync(SvtReceiver& receiver, std::uint64_t expected_time)
    {
        score::ts::GptpIpcData info{};
        info.ptp_assumed_time = std::chrono::nanoseconds{expected_time};
        info.status.is_synchronized = true;
        info.status.is_correct = true;
        info.sync_fup_data.sequence_id = ++sequence_id_;
        info.sync_fup_data.pdelay = 1'000U;
        publisher_.Publish(info);

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
        while (std::chrono::steady_clock::now() < deadline)
        {
            handler_.RunOnce();
            const auto data = receiver.Receive();
            if (data.has_value() && data->ptp_assumed_time == expected_time &&
                data->sync_fup_data.sequence_id == sequence_id_)
            {
                EXPECT_TRUE(data->status.is_synchronized);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }

        FAIL() << "Timed out waiting for vehicle time " << expected_time;
    }

    score::ts::details::GptpIpcPublisher publisher_;
    score::cpp::stop_source stop_source_;
    ScoreVehicleTimebaseHandler handler_;
    std::uint16_t sequence_id_{0U};
};

TEST_F(VehicleTimeHandlerTest, CreateAndDestroy)
{
    auto receiver = CreateSvtReceiver();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
    bool receiver_initialized{false};
    while (std::chrono::steady_clock::now() < deadline)
    {
        handler_.RunOnce();
        receiver_initialized = receiver->Init();
        if (receiver_initialized)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    ASSERT_TRUE(receiver_initialized);

    const std::array<std::uint64_t, 10> expected_times{1234567U,
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
        ASSERT_NO_FATAL_FAILURE(CheckTimeSync(*receiver, expected_time));
    }
}

}  // namespace td
}  // namespace score
