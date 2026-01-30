/********************************************************************************************************
# Copyright ©, 2026, ECARX. All rights reserved. This statement is the current and authoritative version.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: chenhao.gao chenhao.gao@ecarxgroup.com
*********************************************************************************************************/
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tsyncd::ntp
{

    struct Sample
    {
        std::int64_t offset_ns{0};    // server_time - local_time
        std::int64_t delay_ns{0};     // round-trip delay estimate
        std::int64_t mono_rx_ns{0};   // CLOCK_MONOTONIC at receive
        std::int64_t local_tx_ns{0};  // CLOCK_REALTIME at send
        std::int64_t local_rx_ns{0};  // CLOCK_REALTIME at receive
        std::int64_t server_rx_ns{0}; // server receive time (UTC)
        std::int64_t server_tx_ns{0}; // server transmit time (UTC)
    };

    class Client
    {
    public:
        struct Options
        {
            std::vector<std::string> servers = {"pool.ntp.org"};
            int port = 123;
            int timeout_ms = 250;
        };

        explicit Client(Options opt);

        std::optional<Sample> QueryOnce() const;

    private:
        Options opt_;
    };

    struct Estimate
    {
        bool locked{false};
        std::int64_t offset_ns{0};     // filtered offset
        std::int64_t inaccuracy_ns{0}; // best-effort bound
        std::int64_t jitter_ns{0};     // filtered jitter proxy
        std::int64_t last_update_mono_ns{0};
        std::uint32_t good_samples{0};
        std::uint32_t bad_samples{0};
    };

    class Estimator
    {
    public:
        struct Options
        {
            int samples_to_lock = 3;
            double offset_ewma_alpha = 0.2;
            double jitter_ewma_alpha = 0.2;
            std::int64_t max_reasonable_delay_ns = 500'000'000; // 500ms
        };

        explicit Estimator(Options opt);

        void Update(const Sample &s);
        void MarkTimeout(std::int64_t mono_now_ns);
        Estimate Snapshot() const;

    private:
        Options opt_;
        Estimate est_;
    };

} // namespace tsyncd::ntp
