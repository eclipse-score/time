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
#include "score/time/HighPrecisionLocalSteadyClock/high_precision_local_steady_clock_mock.h"
#include "score/time/SynchronizedVehicleTime/details/tdr/receiver.h"

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
    // Create svt publisher to simulate data
    auto publisher = score::td::CreateSvtPublisher("svt_publisher");
    publisher->Init();

    // Simulate expected data;
    const score::td::PtpStatus expected_ptp_status = {true, false, false, false, true};
    const auto cur_local_time = GetTimepoint() - 10ms;
    const score::td::PtpTimeInfo in_data{{1234ns}, cur_local_time, 0, expected_ptp_status, {}, {}};
    publisher->OnMessage(in_data);

    // Create local clock mock to inject proper timestamp
    auto hplsc_mock = std::make_unique<HighPrecisionLocalSteadyClockMock>();
    auto hplsc_mock_raw_ptr = hplsc_mock.get();

    // Given a Receiver instance
    Receiver timebase{std::move(hplsc_mock)};

    // Get timepoint value from high_precision_clock
    const auto local_now_time{GetTimepoint()};

    // Init timebase
    EXPECT_TRUE(timebase.IsAvailable());

    // When calling Now() expect that local clock mock will be invoked, and return mocked data
    EXPECT_CALL(*hplsc_mock_raw_ptr, Now()).WillOnce(Return(local_now_time));
    const auto current_timebase{timebase.Now()};

    // Then timebase status shall be synchronized and time is adjusted to local clock
    const auto expected_time = in_data.ptp_assumed_time + (local_now_time - in_data.local_time);
    EXPECT_EQ(current_timebase.getTimepoint().time_since_epoch(), expected_time);

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
    // Create svt publisher to simulate data
    auto publisher = score::td::CreateSvtPublisher("svt_publisher");
    publisher->Init();

    // Simulate expected data;
    const double expected_deviation_rate = 1.2312;
    publisher->OnMessage({0ns, GetTimepoint(), expected_deviation_rate, {}, {}, {}});

    // Given a Receiver instance
    Receiver timebase{};

    // Init timebase
    EXPECT_TRUE(timebase.IsAvailable());

    // Then GetRateDeviation should return expected rate_deviation
    EXPECT_EQ(timebase.GetRateDeviation(), expected_deviation_rate);
}

TEST_F(TestReceiver, WaitUntilAvailableSucceeds)
{
    // For given a Receiver instance
    Receiver timebase{};

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
    EXPECT_TRUE(timebase.IsAvailable());
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
