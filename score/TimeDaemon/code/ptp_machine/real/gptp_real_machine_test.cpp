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
#include "score/TimeDaemon/code/ptp_machine/real/factory.h"
#include "score/TimeDaemon/code/ptp_machine/real/gptp_real_machine.h"
#include "score/libTSClient/gptp_ipc_publisher.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <mutex>

namespace score
{
namespace td
{

namespace
{

std::string UniqueShmName()
{
    static std::atomic<int> counter{0};
    return "/gptp_rm_it_" + std::to_string(::getpid()) + "_" +
           std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

score::td::PtpTimeInfo MakePublishedInfo()
{
    score::td::PtpTimeInfo info{};
    info.ptp_assumed_time         = std::chrono::nanoseconds{5'000'000'000LL};
    info.rate_deviation           = 0.5;
    info.status.is_synchronized   = true;
    info.status.is_correct        = true;
    info.sync_fup_data.sequence_id = 7U;
    info.sync_fup_data.pdelay      = 1'000U;
    return info;
}

}  // namespace

class GPTPRealMachineIntegrationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        name_ = UniqueShmName();
        ASSERT_TRUE(pub_.Init(name_));
        pub_.Publish(MakePublishedInfo());

        machine_ = CreateGPTPRealMachine("RealPTPMachine", name_);
        machine_->SetPublishCallback([this](const PtpTimeInfo& data) {
            {
                std::lock_guard<std::mutex> lk(mu_);
                published_ = data;
            }
            promise_.set_value();
        });
    }

    void TearDown() override
    {
        machine_->Stop();
        machine_.reset();
        pub_.Destroy();
    }

    std::string                                  name_;
    score::ts::details::GptpIpcPublisher         pub_;
    std::shared_ptr<GPTPRealMachine>             machine_;
    std::promise<void>                           promise_;
    PtpTimeInfo                                  published_{};
    std::mutex                                   mu_;
};

TEST_F(GPTPRealMachineIntegrationTest, GetName_ReturnsConstructionName)
{
    EXPECT_EQ(machine_->GetName(), "RealPTPMachine");
}

TEST_F(GPTPRealMachineIntegrationTest, Init_WhenShmExists_ReturnsTrue)
{
    EXPECT_TRUE(machine_->Init());
}

TEST_F(GPTPRealMachineIntegrationTest, Init_WhenShmMissing_ReturnsFalse)
{
    auto m = CreateGPTPRealMachine("NoShm", "/gptp_nosuchshm_xyz");
    EXPECT_FALSE(m->Init());
}

TEST_F(GPTPRealMachineIntegrationTest, Start_DeliversPublishedData_ViaCallback)
{
    ASSERT_TRUE(machine_->Init());
    machine_->Start();

    auto fut = promise_.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);

    std::lock_guard<std::mutex> lk(mu_);
    EXPECT_EQ(published_.ptp_assumed_time, std::chrono::nanoseconds{5'000'000'000LL});
    EXPECT_DOUBLE_EQ(published_.rate_deviation, 0.5);
    EXPECT_TRUE(published_.status.is_synchronized);
    EXPECT_TRUE(published_.status.is_correct);
    EXPECT_EQ(published_.sync_fup_data.sequence_id, 7U);
    EXPECT_EQ(published_.sync_fup_data.pdelay, 1'000U);
}

TEST_F(GPTPRealMachineIntegrationTest, Init_CalledTwice_SecondCallReturnsSameResult)
{
    ASSERT_TRUE(machine_->Init());
    EXPECT_TRUE(machine_->Init());
}

}  // namespace td
}  // namespace score
