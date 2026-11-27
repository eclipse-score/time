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
#include "score/time_slave/src/gptp/details/network_identity.h"
#include "score/time_slave/src/gptp/details/ptp_types.h"
#include "score/time_slave/src/gptp/details/raw_socket.h"
#include "score/time_slave/src/gptp/gptp_engine.h"
#include "score/ts_client/src/gptp_ipc_publisher.h"
#include "score/ts_client/src/gptp_ipc_receiver.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace score
{
namespace ts
{
namespace details
{
namespace
{

// Raw socket that replays queued frames; Send() is a no-op (Pdelay is out of scope here).
class ReplaySocket final : public RawSocket
{
  public:
    void Push(std::vector<std::uint8_t> data, ::timespec hwts)
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            frames_.emplace_back(std::move(data), hwts);
        }
        cv_.notify_one();
    }

    bool Open(const std::string&) override
    {
        return true;
    }
    bool EnableHwTimestamping() override
    {
        return true;
    }
    void Close() override
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
    }
    int Recv(std::uint8_t* buf, std::size_t buf_len, ::timespec& hwts, int timeout_ms) override
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms > 0 ? timeout_ms : 100), [this] {
            return closed_ || !frames_.empty();
        });
        if (closed_)
        {
            return -1;
        }
        if (frames_.empty())
        {
            return 0;
        }
        auto& [data, ts] = frames_.front();
        const std::size_t n = std::min(data.size(), buf_len);
        std::memcpy(buf, data.data(), n);
        hwts = ts;
        frames_.pop_front();
        return static_cast<int>(n);
    }
    int Send(const void*, int len, ::timespec& hwts) override
    {
        hwts = {};
        return len;
    }
    int GetFd() const override
    {
        return -1;
    }

  private:
    std::deque<std::pair<std::vector<std::uint8_t>, ::timespec>> frames_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool closed_{false};
};

class FixedIdentity final : public NetworkIdentity
{
  public:
    bool Resolve(const std::string&) override
    {
        return true;
    }
    ClockIdentity GetClockIdentity() const override
    {
        ClockIdentity ci{};
        ci.id[0] = 0xAA;
        ci.id[7] = 0xBB;
        return ci;
    }
};

// 14-byte Ethernet header (EtherType 0x88F7) + 34-byte PTP header + 10-byte timestamp body.
std::vector<std::uint8_t> MakeFrame(std::uint8_t msgtype,
                                    std::uint16_t seq_id,
                                    std::uint8_t ctl_field,
                                    std::uint32_t sec_lsb,
                                    std::uint32_t ns)
{
    std::vector<std::uint8_t> f;
    const std::uint8_t dst[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};
    const std::uint8_t src[6] = {0x02, 0x00, 0x00, 0xFF, 0x00, 0x11};
    f.insert(f.end(), dst, dst + 6);
    f.insert(f.end(), src, src + 6);
    f.push_back(0x88);
    f.push_back(0xF7);

    const std::size_t hdr = f.size();
    f.resize(hdr + 34 + 10, 0);
    std::uint8_t* p = f.data() + hdr;
    p[0] = static_cast<std::uint8_t>(0x10U | (msgtype & 0x0FU));
    p[1] = 0x02;
    const std::uint16_t len = htons(static_cast<std::uint16_t>(f.size() - 14));
    std::memcpy(p + 2, &len, 2);
    const std::uint16_t seq = htons(seq_id);
    std::memcpy(p + 30, &seq, 2);
    p[32] = ctl_field;

    const std::uint16_t msb = htons(0U);
    const std::uint32_t sl = htonl(sec_lsb);
    const std::uint32_t n = htonl(ns);
    std::memcpy(p + 34, &msb, 2);
    std::memcpy(p + 36, &sl, 4);
    std::memcpy(p + 40, &n, 4);
    return f;
}

// Process-local unique shared-memory name so parallel test runs do not collide.
std::string UniqueShmName()
{
    static std::atomic<int> counter{0};
    return "/gptp_engine_ipc_it_" + std::to_string(::getpid()) + "_" + std::to_string(counter.fetch_add(1));
}

GptpEngineOptions FastOptions()
{
    GptpEngineOptions o;
    o.iface_name = "lo";
    o.pdelay_warmup_ms = 0;
    o.pdelay_req_interval_ms = 10;
    o.sync_timeout_ms = 3300;
    o.jump_future_threshold_ns = 500'000'000LL;
    return o;
}

}  // namespace

// Verifies the data path TimeSlave::Run() executes every publish cycle:
// Sync/FollowUp frames -> GptpEngine snapshot -> GptpIpcPublisher -> shared memory
// -> GptpIpcReceiver, and that the status flags and timestamps survive the hop.
class GptpEngineIpcPipelineTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        name_ = UniqueShmName();
        auto sock = std::make_unique<ReplaySocket>();
        socket_ = sock.get();
        engine_ = std::make_unique<GptpEngine>(FastOptions(), std::move(sock), std::make_unique<FixedIdentity>());
    }

    void TearDown() override
    {
        engine_->Deinitialize();
        receiver_.Close();
        publisher_.Close();
    }

    // Runs one publish cycle like TimeSlave::Run().
    bool PublishCycle()
    {
        engine_->FinalizeSnapshot();
        score::ts::GptpIpcData data{};
        if (!engine_->ReadPTPSnapshot(data))
        {
            return false;
        }
        publisher_.Publish(data);
        return true;
    }

    std::string name_;
    ReplaySocket* socket_{nullptr};
    std::unique_ptr<GptpEngine> engine_;
    GptpIpcPublisher publisher_;
    GptpIpcReceiver receiver_;
};

TEST_F(GptpEngineIpcPipelineTest, SyncFollowUp_IsPublishedToReceiver)
{
    ASSERT_TRUE(engine_->Initialize());
    ASSERT_TRUE(publisher_.Open(name_));
    ASSERT_TRUE(receiver_.Open(name_));

    ::timespec hwts{1, 0};
    socket_->Push(MakeFrame(kPtpMsgtypeSync, 7U, /*ctl=*/0, 0U, 0U), hwts);
    socket_->Push(MakeFrame(kPtpMsgtypeFollowUp, 7U, /*ctl=*/2, /*sec=*/2U, 0U), {});

    std::optional<score::ts::GptpIpcData> received;
    for (int i = 0; i < 100 && !(received.has_value() && received->status.is_synchronized); ++i)
    {
        ASSERT_TRUE(PublishCycle());
        received = receiver_.Receive();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(received.has_value());
    EXPECT_TRUE(received->status.is_synchronized);
    EXPECT_TRUE(received->status.is_correct);
    EXPECT_FALSE(received->status.is_timeout);
    EXPECT_FALSE(received->status.is_time_jump_future);
    EXPECT_FALSE(received->status.is_time_jump_past);
    EXPECT_GT(received->ptp_assumed_time.count(), 0);
    EXPECT_EQ(received->sync_fup_data.sequence_id, 7U);
}

TEST_F(GptpEngineIpcPipelineTest, TimeJump_IsPublishedToReceiver)
{
    ASSERT_TRUE(engine_->Initialize());
    ASSERT_TRUE(publisher_.Open(name_));
    ASSERT_TRUE(receiver_.Open(name_));

    // Master time advances by 1 s between the two pairs, above the 500 ms threshold.
    socket_->Push(MakeFrame(kPtpMsgtypeSync, 1U, /*ctl=*/0, 0U, 0U), ::timespec{1, 0});
    socket_->Push(MakeFrame(kPtpMsgtypeFollowUp, 1U, /*ctl=*/2, /*sec=*/2U, 0U), {});
    socket_->Push(MakeFrame(kPtpMsgtypeSync, 2U, /*ctl=*/0, 0U, 0U), ::timespec{2, 0});
    socket_->Push(MakeFrame(kPtpMsgtypeFollowUp, 2U, /*ctl=*/2, /*sec=*/3U, 0U), {});

    std::optional<score::ts::GptpIpcData> received;
    for (int i = 0; i < 100 && !(received.has_value() && received->status.is_time_jump_future); ++i)
    {
        ASSERT_TRUE(PublishCycle());
        received = receiver_.Receive();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(received.has_value());
    EXPECT_TRUE(received->status.is_time_jump_future);
    EXPECT_FALSE(received->status.is_time_jump_past);
    // Consumers must still see the flags, so is_correct stays set.
    EXPECT_TRUE(received->status.is_correct);
}

TEST_F(GptpEngineIpcPipelineTest, NoSync_PublishesNotSynchronized)
{
    ASSERT_TRUE(engine_->Initialize());
    ASSERT_TRUE(publisher_.Open(name_));
    ASSERT_TRUE(receiver_.Open(name_));

    ASSERT_TRUE(PublishCycle());
    const auto received = receiver_.Receive();

    ASSERT_TRUE(received.has_value());
    EXPECT_FALSE(received->status.is_synchronized);
}

}  // namespace details
}  // namespace ts
}  // namespace score
