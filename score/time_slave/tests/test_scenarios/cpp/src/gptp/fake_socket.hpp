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
#ifndef SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_FAKE_SOCKET_HPP
#define SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_FAKE_SOCKET_HPP

#include "score/time_slave/src/gptp/details/network_identity.h"
#include "score/time_slave/src/gptp/details/raw_socket.h"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

namespace score
{
namespace ts
{
namespace cit
{

// In-process socket stub: Push() enqueues raw Ethernet frames that the
// GptpEngine RxThread dequeues via Recv().  Send() is a no-op (returns len).
class FakeSocket final : public score::ts::details::RawSocket
{
  public:
    void Push(std::vector<std::uint8_t> data, ::timespec hwts = {})
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            frames_.push_back({std::move(data), hwts});
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
        const auto timeout = std::chrono::milliseconds(timeout_ms > 0 ? timeout_ms : 100);
        cv_.wait_for(lk, timeout, [this] {
            return closed_ || !frames_.empty();
        });
        if (closed_)
            return -1;
        if (frames_.empty())
            return 0;
        auto& [data, ts] = frames_.front();
        const std::size_t n = std::min(data.size(), buf_len);
        std::memcpy(buf, data.data(), n);
        hwts = ts;
        frames_.pop_front();
        return static_cast<int>(n);
    }

    int Send(const void*, int len, ::timespec&) override
    {
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

// Identity stub: always resolves successfully and returns a fixed ClockIdentity.
class FakeIdentity final : public score::ts::details::NetworkIdentity
{
  public:
    bool Resolve(const std::string&) override
    {
        return true;
    }

    score::ts::details::ClockIdentity GetClockIdentity() const override
    {
        score::ts::details::ClockIdentity ci{};
        ci.id[0] = 0xAA;
        ci.id[7] = 0xBB;
        return ci;
    }
};

}  // namespace cit
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_FAKE_SOCKET_HPP
