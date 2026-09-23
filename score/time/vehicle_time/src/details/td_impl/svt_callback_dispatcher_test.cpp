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
#include "score/time/vehicle_time/src/details/td_impl/svt_callback_dispatcher.h"

#include "score/time/vehicle_time/src/details/td_impl/svt_test_helpers.h"
#include "score/time/vehicle_time/src/vehicle_clock.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <optional>
#include <thread>

namespace score
{
namespace time
{
namespace
{

using namespace std::chrono_literals;
using namespace test_helpers;

class SvtCallbackDispatcherTest : public ::testing::Test
{
  protected:
    SvtCallbackDispatcherTest()
        : receiver_{std::make_shared<FakeSvtReceiver>()},
          dispatcher_{std::make_unique<detail::SvtCallbackDispatcher>(receiver_, kPollInterval)}
    {
    }

    void StartServing(const std::optional<score::td::svt::TimeBaseSnapshot>& snapshot)
    {
        receiver_->Serve(snapshot);
        dispatcher_->Start();
    }

    void KeepWorkerPolling()
    {
        dispatcher_->SetPDelayMeasurementFinishedCallback([](const PDelayMeasurementData<VehicleTime>&) {});
    }

    [[nodiscard]] bool WorkerPolledAgain()
    {
        return receiver_->WaitForPolls(5U);
    }

    void ExpectNeverPolled()
    {
        std::this_thread::sleep_for(20 * kPollInterval);
        EXPECT_EQ(receiver_->Polls(), 0U);
    }

    /// Unset() does not synchronise with the worker loop: an iteration that already passed its
    /// "callback registered?" check may still call Receive() once after Unset() returned. So let the
    /// poll counter settle first, then require it to stay put.
    void ExpectNoFurtherPolls()
    {
        auto polls = receiver_->Polls();
        for (std::size_t attempt = 0U; attempt < 50U; ++attempt)
        {
            std::this_thread::sleep_for(5 * kPollInterval);
            const auto now = receiver_->Polls();
            if (now == polls)
            {
                break;
            }
            polls = now;
        }
        std::this_thread::sleep_for(20 * kPollInterval);
        EXPECT_EQ(receiver_->Polls(), polls);
    }

    std::shared_ptr<FakeSvtReceiver> receiver_;
    std::unique_ptr<detail::SvtCallbackDispatcher> dispatcher_;
};

TEST_F(SvtCallbackDispatcherTest, WorkerDoesNotPollWhileNoCallbackIsRegistered)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    ExpectNeverPolled();
}

TEST_F(SvtCallbackDispatcherTest, WorkerDoesNotPollBeforeStartAndDeliversOnceStarted)
{
    receiver_->Serve(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ExpectNeverPolled();

    dispatcher_->Start();
    ASSERT_TRUE(recorder.WaitForCount(1U));
}

TEST_F(SvtCallbackDispatcherTest, WorkerResumesPollingWhenCallbackReRegisteredAfterLastUnset)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));

    dispatcher_->UnsetStatusChangedCallback();
    ExpectNoFurtherPolls();

    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(2U));
}

TEST_F(SvtCallbackDispatcherTest, SettingEmptyCallbackDoesNotInvokeAnything)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    dispatcher_->SetStatusChangedCallback(VehicleTime::StatusChangedCallback{});
    dispatcher_->SetTimeSlaveSyncDataReceivedCallback(VehicleTime::TimeSlaveSyncDataReceivedCallback{});
    dispatcher_->SetPDelayMeasurementFinishedCallback(VehicleTime::PDelayMeasurementFinishedCallback{});

    ExpectNeverPolled();
}

TEST_F(SvtCallbackDispatcherTest, NoCallbackIsDeliveredWhileReceiveReturnsNullopt)
{
    StartServing(std::nullopt);

    Recorder<VehicleTimeStatus> status_recorder;
    Recorder<TimeSlaveSyncData<VehicleTime>> sync_recorder;
    Recorder<PDelayMeasurementData<VehicleTime>> pdelay_recorder;
    dispatcher_->SetStatusChangedCallback(status_recorder.Callback());
    dispatcher_->SetTimeSlaveSyncDataReceivedCallback(sync_recorder.Callback());
    dispatcher_->SetPDelayMeasurementFinishedCallback(pdelay_recorder.Callback());

    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(status_recorder.Count(), 0U);
    EXPECT_EQ(sync_recorder.Count(), 0U);
    EXPECT_EQ(pdelay_recorder.Count(), 0U);
}

TEST_F(SvtCallbackDispatcherTest, DestructorJoinsWorkerWhileCallbacksAreRegistered)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));

    dispatcher_.reset();

    ExpectNoFurtherPolls();
}

TEST_F(SvtCallbackDispatcherTest, DestructorJoinsIdleWorkerAfterLastCallbackUnset)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));
    dispatcher_->UnsetStatusChangedCallback();

    dispatcher_.reset();

    ExpectNoFurtherPolls();
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackFiresOnFirstSnapshotAfterRegistration)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus, 2.5));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());

    ASSERT_TRUE(recorder.WaitForCount(1U));
    EXPECT_TRUE(recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
    EXPECT_FALSE(recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));
    EXPECT_DOUBLE_EQ(recorder.Last().RateDeviation(), 2.5);
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackFiresForInvalidSnapshotWithEmptyFlags)
{
    StartServing(MakeSvtSnapshot(kNotCorrectStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());

    ASSERT_TRUE(recorder.WaitForCount(1U));
    EXPECT_FALSE(recorder.Last().IsConsistent());
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackDoesNotRepeatWhileFlagsAreUnchanged)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());

    ASSERT_TRUE(recorder.WaitForCount(1U));
    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(recorder.Count(), 1U);
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackFiresWhenFlagsChange)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));

    receiver_->Serve(MakeSvtSnapshot(kTimeoutStatus));

    ASSERT_TRUE(recorder.WaitForCount(2U));
    EXPECT_TRUE(recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));
    EXPECT_TRUE(recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackIgnoresRateDeviationChanges)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus, 1.0));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));

    receiver_->Serve(MakeSvtSnapshot(kSynchronizedStatus, 9.0));

    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(recorder.Count(), 1U);
    EXPECT_DOUBLE_EQ(recorder.Last().RateDeviation(), 1.0);
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackReRegisteredReceivesUnchangedStatusAgain)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> first_recorder;
    dispatcher_->SetStatusChangedCallback(first_recorder.Callback());
    ASSERT_TRUE(first_recorder.WaitForCount(1U));

    dispatcher_->UnsetStatusChangedCallback();

    Recorder<VehicleTimeStatus> second_recorder;
    dispatcher_->SetStatusChangedCallback(second_recorder.Callback());

    ASSERT_TRUE(second_recorder.WaitForCount(1U));
    EXPECT_TRUE(second_recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
    EXPECT_EQ(first_recorder.Count(), 1U);

    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(second_recorder.Count(), 1U);
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackReplacedWithoutUnsetReceivesUnchangedStatusAgain)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> first_recorder;
    dispatcher_->SetStatusChangedCallback(first_recorder.Callback());
    ASSERT_TRUE(first_recorder.WaitForCount(1U));

    Recorder<VehicleTimeStatus> second_recorder;
    dispatcher_->SetStatusChangedCallback(second_recorder.Callback());

    ASSERT_TRUE(second_recorder.WaitForCount(1U));
    EXPECT_TRUE(second_recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kSynchronized));
    EXPECT_EQ(first_recorder.Count(), 1U);
}

TEST_F(SvtCallbackDispatcherTest, StatusCallbackRegisteredWhileWorkerIsActiveReceivesCurrentStatusFirst)
{
    StartServing(MakeSvtSnapshot(kTimeoutStatus));

    KeepWorkerPolling();
    ASSERT_TRUE(WorkerPolledAgain());

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());

    ASSERT_TRUE(recorder.WaitForCount(1U));
    EXPECT_TRUE(recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));

    receiver_->Serve(MakeSvtSnapshot(kSynchronizedStatus));
    ASSERT_TRUE(recorder.WaitForCount(2U));
    EXPECT_FALSE(recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));
}

TEST_F(SvtCallbackDispatcherTest, UnsetStatusCallbackStopsDelivery)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));

    dispatcher_->UnsetStatusChangedCallback();
    receiver_->Serve(MakeSvtSnapshot(kTimeoutStatus));

    KeepWorkerPolling();
    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(recorder.Count(), 1U);
}

TEST_F(SvtCallbackDispatcherTest, SyncDataCallbackFiresOnEachNewSnapshot)
{
    auto published = MakeSvtSnapshot(kSynchronizedStatus);
    published.sync_fup_data.sequence_id = 1U;
    StartServing(published);

    Recorder<TimeSlaveSyncData<VehicleTime>> recorder;
    dispatcher_->SetTimeSlaveSyncDataReceivedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));
    EXPECT_EQ(recorder.Last().sequence_id, 1U);

    published.sync_fup_data.sequence_id = 2U;
    receiver_->Serve(published);
    ASSERT_TRUE(recorder.WaitForCount(2U));
    EXPECT_EQ(recorder.Last().sequence_id, 2U);

    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(recorder.Count(), 2U);

    published.sync_fup_data.sequence_id = 3U;
    receiver_->Serve(published);
    ASSERT_TRUE(recorder.WaitForCount(3U));
    EXPECT_EQ(recorder.Last().sequence_id, 3U);
}

TEST_F(SvtCallbackDispatcherTest, SyncDataCallbackReceivesConvertedFields)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<TimeSlaveSyncData<VehicleTime>> recorder;
    dispatcher_->SetTimeSlaveSyncDataReceivedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));

    auto published = MakeSvtSnapshot(kSynchronizedStatus);
    published.sync_fup_data =
        score::td::svt::SyncFupSnapshot{101ULL, 202ULL, 303ULL, 404ULL, 0x10000ULL, 55U, 606ULL, 7U, 0xABCDULL};
    receiver_->Serve(published);

    ASSERT_TRUE(recorder.WaitForCount(2U));
    const auto data = recorder.Last();
    EXPECT_EQ(data.precise_origin_timestamp.time_since_epoch(), 101ns);
    EXPECT_EQ(data.reference_global_timestamp.time_since_epoch(), 202ns);
    EXPECT_EQ(data.reference_local_timestamp.time_since_epoch(), 303ns);
    EXPECT_EQ(data.sync_ingress_timestamp.time_since_epoch(), 404ns);
    EXPECT_EQ(data.correction_field, 0x10000LL);
    EXPECT_EQ(data.sequence_id, 55U);
    EXPECT_EQ(data.pdelay, 606ns);
    EXPECT_EQ(data.source_port_identity.port_number, 7U);
    EXPECT_EQ(data.source_port_identity.clock_identity, 0xABCDULL);
}

TEST_F(SvtCallbackDispatcherTest, PDelayCallbackFiresOnEachNewSnapshot)
{
    auto published = MakeSvtSnapshot(kSynchronizedStatus);
    published.pdelay_data.sequence_id = 1U;
    StartServing(published);

    Recorder<PDelayMeasurementData<VehicleTime>> recorder;
    dispatcher_->SetPDelayMeasurementFinishedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));
    EXPECT_EQ(recorder.Last().sequence_id, 1U);

    published.pdelay_data.sequence_id = 2U;
    receiver_->Serve(published);
    ASSERT_TRUE(recorder.WaitForCount(2U));
    EXPECT_EQ(recorder.Last().sequence_id, 2U);

    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(recorder.Count(), 2U);

    published.pdelay_data.sequence_id = 3U;
    receiver_->Serve(published);
    ASSERT_TRUE(recorder.WaitForCount(3U));
    EXPECT_EQ(recorder.Last().sequence_id, 3U);
}

TEST_F(SvtCallbackDispatcherTest, PDelayCallbackReceivesConvertedFields)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<PDelayMeasurementData<VehicleTime>> recorder;
    dispatcher_->SetPDelayMeasurementFinishedCallback(recorder.Callback());
    ASSERT_TRUE(recorder.WaitForCount(1U));

    auto published = MakeSvtSnapshot(kSynchronizedStatus);
    published.pdelay_data = score::td::svt::PDelayDataSnapshot{
        11ULL, 22ULL, 33ULL, 44ULL, 55ULL, 66ULL, 77U, 88ULL, 3U, 0x1111ULL, 4U, 0x2222ULL};
    receiver_->Serve(published);

    ASSERT_TRUE(recorder.WaitForCount(2U));
    const auto data = recorder.Last();
    EXPECT_EQ(data.request_origin_timestamp.time_since_epoch(), 11ns);
    EXPECT_EQ(data.request_receipt_timestamp.time_since_epoch(), 22ns);
    EXPECT_EQ(data.response_origin_timestamp.time_since_epoch(), 33ns);
    EXPECT_EQ(data.response_receipt_timestamp.time_since_epoch(), 44ns);
    EXPECT_EQ(data.reference_global_timestamp.time_since_epoch(), 55ns);
    EXPECT_EQ(data.reference_local_timestamp.time_since_epoch(), 66ns);
    EXPECT_EQ(data.sequence_id, 77U);
    EXPECT_EQ(data.pdelay, 88ns);
    EXPECT_EQ(data.request_port_identity.port_number, 3U);
    EXPECT_EQ(data.request_port_identity.clock_identity, 0x1111ULL);
    EXPECT_EQ(data.response_port_identity.port_number, 4U);
    EXPECT_EQ(data.response_port_identity.clock_identity, 0x2222ULL);
}

TEST_F(SvtCallbackDispatcherTest, AllThreeCallbacksAreDeliveredFromTheSameSnapshot)
{
    auto published = MakeSvtSnapshot(kSynchronizedStatus);
    StartServing(published);

    Recorder<VehicleTimeStatus> status_recorder;
    Recorder<TimeSlaveSyncData<VehicleTime>> sync_recorder;
    Recorder<PDelayMeasurementData<VehicleTime>> pdelay_recorder;
    dispatcher_->SetTimeSlaveSyncDataReceivedCallback(sync_recorder.Callback());
    dispatcher_->SetPDelayMeasurementFinishedCallback(pdelay_recorder.Callback());
    dispatcher_->SetStatusChangedCallback(status_recorder.Callback());
    ASSERT_TRUE(status_recorder.WaitForCount(1U));
    ASSERT_TRUE(sync_recorder.WaitForCount(1U));
    ASSERT_TRUE(pdelay_recorder.WaitForCount(1U));

    published.status = kTimeoutStatus;
    published.sync_fup_data.sequence_id = 5U;
    published.pdelay_data.sequence_id = 6U;
    receiver_->Serve(published);

    ASSERT_TRUE(status_recorder.WaitForCount(2U));
    ASSERT_TRUE(sync_recorder.WaitForCount(2U));
    ASSERT_TRUE(pdelay_recorder.WaitForCount(2U));
    EXPECT_TRUE(status_recorder.Last().IsFlagActive(VehicleTime::StatusFlag::kTimeOut));
    EXPECT_EQ(sync_recorder.Last().sequence_id, 5U);
    EXPECT_EQ(pdelay_recorder.Last().sequence_id, 6U);
}

TEST_F(SvtCallbackDispatcherTest, ChangeInOneSnapshotPartFiresOnlyThatCallback)
{
    auto published = MakeSvtSnapshot(kSynchronizedStatus);
    StartServing(published);

    Recorder<VehicleTimeStatus> status_recorder;
    Recorder<TimeSlaveSyncData<VehicleTime>> sync_recorder;
    Recorder<PDelayMeasurementData<VehicleTime>> pdelay_recorder;
    dispatcher_->SetTimeSlaveSyncDataReceivedCallback(sync_recorder.Callback());
    dispatcher_->SetPDelayMeasurementFinishedCallback(pdelay_recorder.Callback());
    dispatcher_->SetStatusChangedCallback(status_recorder.Callback());
    ASSERT_TRUE(status_recorder.WaitForCount(1U));
    ASSERT_TRUE(sync_recorder.WaitForCount(1U));
    ASSERT_TRUE(pdelay_recorder.WaitForCount(1U));

    published.sync_fup_data.sequence_id = 5U;
    receiver_->Serve(published);
    ASSERT_TRUE(sync_recorder.WaitForCount(2U));
    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(status_recorder.Count(), 1U);
    EXPECT_EQ(pdelay_recorder.Count(), 1U);

    published.pdelay_data.sequence_id = 6U;
    receiver_->Serve(published);
    ASSERT_TRUE(pdelay_recorder.WaitForCount(2U));
    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(status_recorder.Count(), 1U);
    EXPECT_EQ(sync_recorder.Count(), 2U);
}

TEST_F(SvtCallbackDispatcherTest, UnsetFromWithinCallbackDoesNotDeadlock)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    Recorder<VehicleTimeStatus> recorder;
    dispatcher_->SetStatusChangedCallback([this, &recorder](const VehicleTimeStatus& status) {
        recorder.Record(status);
        dispatcher_->UnsetStatusChangedCallback();
    });
    ASSERT_TRUE(recorder.WaitForCount(1U));

    KeepWorkerPolling();
    receiver_->Serve(MakeSvtSnapshot(kTimeoutStatus));

    ASSERT_TRUE(WorkerPolledAgain());
    EXPECT_EQ(recorder.Count(), 1U);
}

TEST_F(SvtCallbackDispatcherTest, SubscribingToAnotherEventFromWithinCallbackDoesNotDeadlock)
{
    auto published = MakeSvtSnapshot(kSynchronizedStatus);
    StartServing(published);

    Recorder<VehicleTimeStatus> status_recorder;
    Recorder<TimeSlaveSyncData<VehicleTime>> sync_recorder;
    dispatcher_->SetStatusChangedCallback([this, &status_recorder, &sync_recorder](const VehicleTimeStatus& status) {
        status_recorder.Record(status);
        dispatcher_->SetTimeSlaveSyncDataReceivedCallback(sync_recorder.Callback());
    });
    ASSERT_TRUE(status_recorder.WaitForCount(1U));
    ASSERT_TRUE(sync_recorder.WaitForCount(1U));

    published.sync_fup_data.sequence_id = 42U;
    receiver_->Serve(published);

    ASSERT_TRUE(sync_recorder.WaitForCount(2U));
    EXPECT_EQ(sync_recorder.Last().sequence_id, 42U);
}

TEST_F(SvtCallbackDispatcherTest, UnsetBlocksUntilInFlightCallbackReturns)
{
    StartServing(MakeSvtSnapshot(kSynchronizedStatus));

    std::promise<void> callback_entered;
    std::promise<void> release_callback;
    auto release_future = release_callback.get_future().share();
    dispatcher_->SetStatusChangedCallback([&callback_entered, release_future](const VehicleTimeStatus&) {
        callback_entered.set_value();
        release_future.wait();
    });
    ASSERT_EQ(callback_entered.get_future().wait_for(kWaitTimeout), std::future_status::ready);

    auto unset_done = std::async(std::launch::async, [this]() {
        dispatcher_->UnsetStatusChangedCallback();
    });
    EXPECT_EQ(unset_done.wait_for(50 * kPollInterval), std::future_status::timeout);

    release_callback.set_value();
    EXPECT_EQ(unset_done.wait_for(kWaitTimeout), std::future_status::ready);
}

}  // namespace
}  // namespace time
}  // namespace score
