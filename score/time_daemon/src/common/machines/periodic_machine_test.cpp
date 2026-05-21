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
#include "score/time_daemon/src/common/machines/periodic_machine.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

namespace score
{
namespace td
{
namespace test
{
/**
 * brief Simple interface to mock the PeriodicTask() method of PeriodicMachine
 */
class SimpleJobShower
{
  public:
    virtual ~SimpleJobShower() = default;

    virtual void ShowJob() = 0;
};

/**
 * brief Mock class to test the PeriodicMachine class
 */
class SimpleJobShowerMock : public SimpleJobShower
{
  public:
    MOCK_METHOD(void, ShowJob, (), (override));
};

/**
 * brief Fake class to test the PeriodicMachine class
 */
class PeriodicMachineFake : public PeriodicMachine
{
  public:
    explicit PeriodicMachineFake(const std::string& name,
                                 const std::chrono::milliseconds threadCycle,
                                 SimpleJobShower* jobShower)
        : PeriodicMachine(name, threadCycle), job_shower_(jobShower)
    {
    }

    ~PeriodicMachineFake() override = default;

    void PeriodicTask() noexcept override
    {
        job_shower_->ShowJob();
    }

    bool Init() override
    {
        return true;
    }

  private:
    SimpleJobShower* job_shower_;
};

}  // namespace test

class PeriodicMachineTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        sut_ =
            std::make_unique<test::PeriodicMachineFake>("TestMachine", std::chrono::milliseconds(100), &jobShowerMock_);
    }

    void TearDown() override
    {
        sut_.reset();
    }

  protected:
    ::testing::StrictMock<test::SimpleJobShowerMock> jobShowerMock_;
    std::unique_ptr<test::PeriodicMachineFake> sut_;
};

TEST_F(PeriodicMachineTest, StartAndStop)
{
    EXPECT_CALL(jobShowerMock_, ShowJob()).Times(::testing::AtLeast(1));

    sut_->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sut_->Stop();
}

TEST_F(PeriodicMachineTest, NoNotificationsWithoutStart)
{
    EXPECT_CALL(jobShowerMock_, ShowJob()).Times(0);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(PeriodicMachineTest, NoNotificationsAfterStop)
{
    EXPECT_CALL(jobShowerMock_, ShowJob()).Times(::testing::AtLeast(2));

    sut_->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sut_->Stop();

    EXPECT_CALL(jobShowerMock_, ShowJob()).Times(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(PeriodicMachineTest, NoActionsWithoutStartButWithStop)
{
    EXPECT_CALL(jobShowerMock_, ShowJob()).Times(0);

    sut_->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

}  // namespace td
}  // namespace score
