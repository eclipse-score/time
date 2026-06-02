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
#include "score/ts_client/src/gptp_ipc_channel.h"
#include "score/ts_client/src/gptp_ipc_publisher.h"
#include "score/ts_client/src/gptp_ipc_receiver.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

// Generate a unique POSIX shm name per invocation (avoids cross-test pollution).
std::string UniqueShmName()
{
    static std::atomic<int> counter{0};
    return "/gptp_ipc_ut_" + std::to_string(::getpid()) + "_" +
           std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

// RAII helper: creates shm manually (without GptpIpcPublisher) for edge-case
// testing; cleans up in destructor.
struct ManualShm
{
    std::string name;
    void* ptr = MAP_FAILED;
    std::size_t size = sizeof(GptpIpcRegion);

    explicit ManualShm(const std::string& n) : name{n}
    {
        const int fd = ::shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd < 0)
            return;
        if (::ftruncate(fd, static_cast<off_t>(size)) != 0)
        {
            ::close(fd);
            return;
        }
        ptr = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ::close(fd);
    }

    ~ManualShm()
    {
        if (ptr != MAP_FAILED)
            ::munmap(ptr, size);
        ::shm_unlink(name.c_str());
    }

    bool Valid() const
    {
        return ptr != MAP_FAILED;
    }
    GptpIpcRegion* Region()
    {
        return static_cast<GptpIpcRegion*>(ptr);
    }
};

}  // namespace

// ── GptpIpcPublisher ──────────────────────────────────────────────────────────

class GptpIpcPublisherTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        pub_.Destroy();
    }

    GptpIpcPublisher pub_;
};

TEST_F(GptpIpcPublisherTest, Init_ValidName_ReturnsTrue)
{
    EXPECT_TRUE(pub_.Init(UniqueShmName()));
}

TEST_F(GptpIpcPublisherTest, Publish_WithoutInit_DoesNotCrash)
{
    // region_ is nullptr; Publish() must return silently.
    score::td::PtpTimeInfo info{};
    EXPECT_NO_THROW(pub_.Publish(info));
}

TEST_F(GptpIpcPublisherTest, Destroy_CalledTwice_DoesNotCrash)
{
    ASSERT_TRUE(pub_.Init(UniqueShmName()));
    pub_.Destroy();
    EXPECT_NO_THROW(pub_.Destroy());
}

TEST_F(GptpIpcPublisherTest, Destroy_WithoutInit_DoesNotCrash)
{
    EXPECT_NO_THROW(pub_.Destroy());
}

TEST_F(GptpIpcPublisherTest, Init_CalledTwice_ReturnsTrueOnSecondCall)
{
    // region_ != nullptr after first Init → second call returns true immediately.
    ASSERT_TRUE(pub_.Init(UniqueShmName()));
    EXPECT_TRUE(pub_.Init(UniqueShmName()));
}

// ── GptpIpcReceiver ───────────────────────────────────────────────────────────

class GptpIpcReceiverTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        rx_.Close();
    }

    GptpIpcReceiver rx_;
};

TEST_F(GptpIpcReceiverTest, Init_ShmNotExist_ReturnsFalse)
{
    EXPECT_FALSE(rx_.Init("/gptp_nonexistent_" + std::to_string(::getpid())));
}

TEST_F(GptpIpcReceiverTest, Close_WithoutInit_DoesNotCrash)
{
    EXPECT_NO_THROW(rx_.Close());
}

TEST_F(GptpIpcReceiverTest, Close_CalledTwice_DoesNotCrash)
{
    EXPECT_NO_THROW(rx_.Close());
    EXPECT_NO_THROW(rx_.Close());
}

TEST_F(GptpIpcReceiverTest, Receive_WithoutInit_ReturnsNullopt)
{
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcReceiverTest, Init_CalledTwice_ReturnsTrueOnSecondCall)
{
    // region_ != nullptr after first Init → second call returns true immediately.
    GptpIpcPublisher pub;
    const std::string name = UniqueShmName();
    ASSERT_TRUE(pub.Init(name));
    ASSERT_TRUE(rx_.Init(name));
    EXPECT_TRUE(rx_.Init(name));
    pub.Destroy();
}

TEST_F(GptpIpcReceiverTest, Init_TooSmallShm_ReturnsFalse)
{
    // Create a shm segment smaller than GptpIpcRegion so the fstat size check fails.
    const std::string name = UniqueShmName();
    const int fd = ::shm_open(name.c_str(), O_CREAT | O_RDWR, 0600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(::ftruncate(fd, 1), 0);
    ::close(fd);

    EXPECT_FALSE(rx_.Init(name));

    ::shm_unlink(name.c_str());
}

// ── Publisher + Receiver roundtrip ────────────────────────────────────────────

class GptpIpcRoundtripTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        name_ = UniqueShmName();
    }
    void TearDown() override
    {
        rx_.Close();
        pub_.Destroy();
    }

    std::string name_;
    GptpIpcPublisher pub_;
    GptpIpcReceiver rx_;
};

TEST_F(GptpIpcRoundtripTest, ReceiverInit_AfterPublisherInit_ReturnsTrue)
{
    ASSERT_TRUE(pub_.Init(name_));
    EXPECT_TRUE(rx_.Init(name_));
}

TEST_F(GptpIpcRoundtripTest, ReceiverReceive_BeforeAnyPublish_ReturnsNullopt)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));
    // seq_confirm is initialised to 1 (≠ seq=0) by GptpIpcRegion's constructor,
    // so the seqlock always mismatches before the first Publish() call.
    // Receive() must exhaust its retries and return std::nullopt.
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_BasicFields_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::td::PtpTimeInfo info{};
    info.ptp_assumed_time = std::chrono::nanoseconds{1'234'567'890LL};
    info.rate_deviation = 0.75;
    info.status.is_synchronized = true;
    info.status.is_correct = true;

    pub_.Publish(info);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ptp_assumed_time, info.ptp_assumed_time);
    EXPECT_DOUBLE_EQ(result->rate_deviation, info.rate_deviation);
    EXPECT_TRUE(result->status.is_synchronized);
    EXPECT_TRUE(result->status.is_correct);
    EXPECT_FALSE(result->status.is_timeout);
    EXPECT_FALSE(result->status.is_time_jump_future);
    EXPECT_FALSE(result->status.is_time_jump_past);
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_StatusFlags_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::td::PtpTimeInfo info{};
    info.status.is_timeout = true;
    info.status.is_time_jump_future = true;
    info.status.is_time_jump_past = false;
    info.status.is_synchronized = false;
    info.status.is_correct = false;

    pub_.Publish(info);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->status.is_timeout);
    EXPECT_TRUE(result->status.is_time_jump_future);
    EXPECT_FALSE(result->status.is_time_jump_past);
    EXPECT_FALSE(result->status.is_synchronized);
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_SyncFupData_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::td::PtpTimeInfo info{};
    info.sync_fup_data.precise_origin_timestamp = 100'000'000'000ULL;
    info.sync_fup_data.reference_global_timestamp = 100'000'001'000ULL;
    info.sync_fup_data.reference_local_timestamp = 100'000'001'500ULL;
    info.sync_fup_data.sync_ingress_timestamp = 100'000'001'500ULL;
    info.sync_fup_data.correction_field = 42U;
    info.sync_fup_data.sequence_id = 77;
    info.sync_fup_data.pdelay = 3'000U;
    info.sync_fup_data.port_number = 1;
    info.sync_fup_data.clock_identity = 0xAABBCCDDEEFF0011ULL;

    pub_.Publish(info);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->sync_fup_data.precise_origin_timestamp, 100'000'000'000ULL);
    EXPECT_EQ(result->sync_fup_data.reference_global_timestamp, 100'000'001'000ULL);
    EXPECT_EQ(result->sync_fup_data.sequence_id, 77);
    EXPECT_EQ(result->sync_fup_data.pdelay, 3'000U);
    EXPECT_EQ(result->sync_fup_data.clock_identity, 0xAABBCCDDEEFF0011ULL);
}

TEST_F(GptpIpcRoundtripTest, PublishReceive_PDelayData_RoundtripCorrectly)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    score::td::PtpTimeInfo info{};
    info.pdelay_data.request_origin_timestamp = 1'000'000'000ULL;
    info.pdelay_data.request_receipt_timestamp = 1'000'001'000ULL;
    info.pdelay_data.response_origin_timestamp = 1'000'001'000ULL;
    info.pdelay_data.response_receipt_timestamp = 1'000'002'000ULL;
    info.pdelay_data.pdelay = 1'000U;
    info.pdelay_data.req_port_number = 1;
    info.pdelay_data.resp_port_number = 2;
    info.pdelay_data.req_clock_identity = 0x1122334455667788ULL;

    pub_.Publish(info);

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pdelay_data.request_origin_timestamp, 1'000'000'000ULL);
    EXPECT_EQ(result->pdelay_data.pdelay, 1'000U);
    EXPECT_EQ(result->pdelay_data.req_port_number, 1);
    EXPECT_EQ(result->pdelay_data.resp_port_number, 2);
    EXPECT_EQ(result->pdelay_data.req_clock_identity, 0x1122334455667788ULL);
}

TEST_F(GptpIpcRoundtripTest, MultiplePublish_LastValueIsVisible)
{
    ASSERT_TRUE(pub_.Init(name_));
    ASSERT_TRUE(rx_.Init(name_));

    for (int i = 1; i <= 5; ++i)
    {
        score::td::PtpTimeInfo info{};
        info.ptp_assumed_time = std::chrono::nanoseconds{static_cast<std::int64_t>(i) * 1'000'000'000LL};
        pub_.Publish(info);
    }

    const auto result = rx_.Receive();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ptp_assumed_time, std::chrono::nanoseconds{5'000'000'000LL});
}

// ── Edge cases via ManualShm ──────────────────────────────────────────────────

TEST_F(GptpIpcRoundtripTest, ReceiverInit_WrongMagic_ReturnsFalse)
{
    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    // Placement-new initializes magic = kGptpIpcMagic; overwrite with bad value.
    new (shm.Region()) GptpIpcRegion{};
    const std::uint32_t bad = 0xDEADBEEFU;
    std::memcpy(shm.ptr, &bad, sizeof(bad));

    EXPECT_FALSE(rx_.Init(name_));
}

TEST_F(GptpIpcRoundtripTest, Receive_PersistentOddSeq_ExhaustsRetriesAndReturnsNullopt)
{
    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    // seq=1 (odd = writer active), seq_confirm=0; seqlock never resolves.
    auto* region = new (shm.Region()) GptpIpcRegion{};
    region->seq.store(1U, std::memory_order_relaxed);
    region->seq_confirm.store(0U, std::memory_order_relaxed);

    ASSERT_TRUE(rx_.Init(name_));
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcRoundtripTest, Receive_SeqConfirmMismatch_ExhaustsRetriesAndReturnsNullopt)
{
    ManualShm shm{name_};
    ASSERT_TRUE(shm.Valid());

    // seq=4 (even, not writing) but seq_confirm=2 → mismatch: write still pending.
    auto* region = new (shm.Region()) GptpIpcRegion{};
    region->seq.store(4U, std::memory_order_relaxed);
    region->seq_confirm.store(2U, std::memory_order_relaxed);

    ASSERT_TRUE(rx_.Init(name_));
    EXPECT_FALSE(rx_.Receive().has_value());
}

}  // namespace details
}  // namespace ts
}  // namespace score
