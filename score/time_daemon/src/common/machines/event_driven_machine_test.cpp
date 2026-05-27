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
#include "score/time_daemon/src/common/machines/event_driven_machine.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

namespace score
{
namespace td
{
namespace test
{
/**
 * brief Simple interface to mock the PeriodicTask() method of EventDrivenMachine
 */
class EventJobShower
{
  public:
    virtual ~EventJobShower() = default;

    virtual void OnEvent() = 0;
    virtual void OnTimeout() = 0;
};

/**
 * brief Mock class to test the EventDrivenMachine class
 */
class EventJobShowerMock : public EventJobShower
{
  public:
    MOCK_METHOD(void, OnEvent, (), (override));
    MOCK_METHOD(void, OnTimeout, (), (override));
};

/**
 * brief Fake class to test the EventDrivenMachine class
 */
class EventDrivenMachineFake : public EventDrivenMachine
{
  public:
    explicit EventDrivenMachineFake(const std::string& name,
                                    const std::chrono::milliseconds threadCycle,
                                    EventJobShower* jobShower)
        : EventDrivenMachine(name, threadCycle), job_shower_(jobShower)
    {
    }

    ~EventDrivenMachineFake() override = default;

    void OnEvent() noexcept override
    {
        job_shower_->OnEvent();
    }

    void OnTimeout() noexcept override
    {
        job_shower_->OnTimeout();
    }

    bool Init() override
    {
        return true;
    }

  private:
    EventJobShower* job_shower_;
};

}  // namespace test

class EventDrivenMachineTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        sut_ = std::make_unique<test::EventDrivenMachineFake>(
            "TestMachine", std::chrono::milliseconds(100), &job_shower_mock_);
    }

    void TearDown() override
    {
        sut_.reset();
    }

  protected:
    ::testing::StrictMock<test::EventJobShowerMock> job_shower_mock_;
    std::unique_ptr<test::EventDrivenMachineFake> sut_;
};

TEST_F(EventDrivenMachineTest, StartAndStop)
{
    EXPECT_CALL(job_shower_mock_, OnEvent()).Times(0);
    EXPECT_CALL(job_shower_mock_, OnTimeout()).Times(::testing::AtLeast(3));

    sut_->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sut_->Stop();
}

TEST_F(EventDrivenMachineTest, NoNotificationsWithoutStart)
{
    EXPECT_CALL(job_shower_mock_, OnEvent()).Times(0);
    EXPECT_CALL(job_shower_mock_, OnTimeout()).Times(0);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(EventDrivenMachineTest, NoNotificationsAfterStop)
{
    EXPECT_CALL(job_shower_mock_, OnEvent()).Times(0);
    EXPECT_CALL(job_shower_mock_, OnTimeout()).Times(::testing::AtLeast(2));

    sut_->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sut_->Stop();

    EXPECT_CALL(job_shower_mock_, OnEvent()).Times(0);
    EXPECT_CALL(job_shower_mock_, OnTimeout()).Times(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(EventDrivenMachineTest, NoActionsWithoutStartButWithStop)
{
    EXPECT_CALL(job_shower_mock_, OnEvent()).Times(0);
    EXPECT_CALL(job_shower_mock_, OnTimeout()).Times(0);

    sut_->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(EventDrivenMachineTest, NotificationOnEvent)
{
    EXPECT_CALL(job_shower_mock_, OnEvent()).Times(1);
    EXPECT_CALL(job_shower_mock_, OnTimeout()).Times(::testing::AtLeast(1));

    sut_->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sut_->NotifyEvent();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sut_->Stop();
}

}  // namespace td
}  // namespace score
