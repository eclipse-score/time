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
#include "score/time/SynchronizedVehicleTime/details/tdr/receiver.h"

#include "score/time_daemon/src/ipc/receiver_mock.h"

#include "score/time_daemon/src/ipc/svt/receiver/factory.h"

#include "score/time_daemon/src/ipc/svt/receiver/svt_receiver.h"

#include "score/time/HighPrecisionLocalSteadyClock/high_precision_local_steady_clock_mock.h"

#include <gtest/gtest.h>
#include <chrono>

namespace score
{
namespace time
{
namespace details
{
namespace tdr
{

using ::testing::_;
using ::testing::Return;

using namespace std::chrono_literals;

using StatusFlag = SynchronizedVehicleTime::StatusFlag;

score::time::HighPrecisionLocalSteadyClock::time_point GetTimepoint()
{
    return score::time::HighPrecisionLocalSteadyClock::time_point{123456789ns};
}

class TestReceiver : public ::testing::Test
{
  public:
    void SetUp() override
    {
        auto vtd_ipc_receiver_base = score::td::CreateSvtReceiver();
        vtd_ipc_receiver_mock =
            std::dynamic_pointer_cast<score::td::ReceiverMock<score::td::svt::TimeBaseSnapshot>>(
                vtd_ipc_receiver_base);
        ASSERT_NE(vtd_ipc_receiver_mock, nullptr);
    }

    void TearDown() override
    {
        vtd_ipc_receiver_mock.reset();
    }

    std::shared_ptr<score::td::ReceiverMock<score::td::svt::TimeBaseSnapshot>> vtd_ipc_receiver_mock;
};

TEST_F(TestReceiver, Destruction)
{
    // Given an instance via base-class pointer
    std::unique_ptr<SynchronizedVehicleTime> slave_base = std::make_unique<Receiver>();
    // When being reset
    slave_base.reset();
    // Then it gets destructed

    // Given an instance via direct pointer
    auto slave_impl = std::make_unique<Receiver>();
    // When being reset
    slave_impl.reset();
    // Then it gets destructed

    {
        // Given an instance via object on stack
        Receiver timebase{};
        // When going out of scope
        (void)timebase;
    }
    // Then it gets destructed
}

TEST_F(TestReceiver, GetCurrentTimeAndStatusSynchronized)
{
    // Create local clock mock to inject proper timestamp
    auto hplsc_mock = std::make_unique<HighPrecisionLocalSteadyClockMock>();
    auto hplsc_mock_raw_ptr = hplsc_mock.get();

    // Given a Receiver instance
    Receiver timebase{std::move(hplsc_mock)};

    // Get timepoint value from high_precision_clock
    const auto local_now_time{GetTimepoint()};

    // Init timebase and that vtd_ipc_receiver_mock will be called and return true
    EXPECT_CALL(*vtd_ipc_receiver_mock, Init()).WillOnce(Return(true));
    EXPECT_TRUE(timebase.IsAvailable());

    // When calling Now() expect that local clock mock will be invoked, and return mocked data with simulated input data
    EXPECT_CALL(*hplsc_mock_raw_ptr, Now()).WillOnce(Return(local_now_time));

    const score::td::svt::TimeBaseStatus expected_ptp_status = {true, false, false, false, true};
    const score::td::svt::TimeBaseSnapshot in_data{1234, 5678, 0, expected_ptp_status, {}, {}};

    EXPECT_CALL(*vtd_ipc_receiver_mock, Receive()).WillOnce(Return(in_data));
    const auto current_timebase{timebase.Now()};

    // Then timebase status shall be synchronized and time is adjusted to local clock
    const auto expected_time =
        in_data.ptp_assumed_time + (local_now_time.time_since_epoch().count() - in_data.local_time);
    EXPECT_EQ(current_timebase.getTimepoint().time_since_epoch().count(), expected_time);

    EXPECT_TRUE(current_timebase.getTimepointStatus().IsFlagActive(StatusFlag::kSynchronized));
    EXPECT_FALSE(current_timebase.getTimepointStatus().IsAnyOfFlagsActive({StatusFlag::kUnknown,
                                                                           StatusFlag::kTimeOut,
                                                                           StatusFlag::kTimeLeapPast,
                                                                           StatusFlag::kSynchToGateway,
                                                                           StatusFlag::kTimeLeapFuture}));
    EXPECT_TRUE(current_timebase.getTimepointStatus().IsSynchronized());
}

TEST_F(TestReceiver, CheckDefaultRateDeviation)
{
    // Given a Receiver instance
    Receiver timebase{};

    // Then GetRateDeviation should return expected rate_deviation when no data was injected or timebase is not
    // available
    EXPECT_EQ(timebase.GetRateDeviation(), 0.0);
}

TEST_F(TestReceiver, CheckSimulatedRateDeviation)
{
    // Given a Receiver instance
    Receiver timebase{};

    // Init timebase and that vtd_ipc_receiver_mock will be called and return true
    EXPECT_CALL(*vtd_ipc_receiver_mock, Init()).WillOnce(Return(true));
    EXPECT_TRUE(timebase.IsAvailable());

    // Then GetRateDeviation should return expected rate_deviation
    const double expected_deviation_rate = 1.2312;
    EXPECT_CALL(*vtd_ipc_receiver_mock, Receive())
        .WillOnce(Return(score::td::svt::TimeBaseSnapshot{0, 0, expected_deviation_rate, {}, {}, {}}));
    EXPECT_EQ(timebase.GetRateDeviation(), expected_deviation_rate);
}

TEST_F(TestReceiver, WaitUntilAvailableSucceeds)
{
    // For given a Receiver instance
    Receiver timebase{};

    EXPECT_CALL(*vtd_ipc_receiver_mock, Init()).WillOnce(Return(true));
    // When its method WaitUntilAvailable() gets called with a timepoint in the future, then true must be returned
    const auto current_time = std::chrono::steady_clock::now();
    EXPECT_TRUE(timebase.WaitUntilAvailable(score::cpp::stop_source{}.get_token(), current_time + 3s));

    // When its method WaitUntilAvailable() gets called again with a timepoint in the future, then true must be returned
    EXPECT_TRUE(timebase.WaitUntilAvailable(score::cpp::stop_source{}.get_token(), current_time + 7s));
}

TEST_F(TestReceiver, IsAvailableSucceeds)
{
    // Given a Receiver instance
    Receiver timebase{};

    // When its method IsAvailable(), true must be returned
    EXPECT_CALL(*vtd_ipc_receiver_mock, Init()).WillOnce(Return(true));
    EXPECT_TRUE(timebase.IsAvailable());
}

TEST_F(TestReceiver, IsAvailableFails)
{
    // Given a Receiver instance
    Receiver timebase{};

    // When its method IsAvailable(), true must be returned
    EXPECT_CALL(*vtd_ipc_receiver_mock, Init()).WillOnce(Return(false));
    EXPECT_FALSE(timebase.IsAvailable());
}

TEST_F(TestReceiver, CallAllEmptyMethods)
{
    // Given a Receiver instance
    Receiver timebase{};

    // When its methods ar called, nothing is returned
    timebase.SetTimeSlaveSyncDataReceivedCallback([](TimeSlaveSyncData<SynchronizedVehicleTime>) {});
    timebase.UnsetTimeSlaveSyncDataReceivedCallback();
    timebase.SetPDelayMeasurementFinishedCallback([](PDelayMeasurementData<SynchronizedVehicleTime>) {});
    timebase.UnsetPDelayMeasurementFinishedCallback();
}

}  // namespace tdr
}  // namespace details
}  // namespace time
}  // namespace score
