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
#include "score/time_slave/src/gptp/phc/phc_adjuster.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/timex.h>
#include <syscall.h>
#include <unistd.h>
#include <cmath>
#include <cstdlib>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

// clock_adjtime is not always exposed via glibc headers in cross-compilers.
// Use the syscall directly.
int phc_clock_adjtime(clockid_t clk_id, struct timex* tx)
{
    return static_cast<int>(::syscall(SYS_clock_adjtime, clk_id, tx));
}

// Construct a clockid from a PHC file descriptor (kernel convention).
clockid_t phc_fd_to_clockid(int fd)
{
    return static_cast<clockid_t>((~static_cast<unsigned int>(fd) << 3U) | 3U);
}

}  // namespace

PhcAdjuster::PhcAdjuster(PhcConfig cfg) : cfg_{std::move(cfg)}
{
    if (cfg_.enabled && !cfg_.device.empty())
    {
        phc_fd_ = ::open(cfg_.device.c_str(), O_RDWR);
    }
}

PhcAdjuster::~PhcAdjuster() noexcept
{
    if (phc_fd_ >= 0)
    {
        ::close(phc_fd_);
        phc_fd_ = -1;
    }
}

void PhcAdjuster::AdjustOffset(std::int64_t offset_ns)
{
    if (!cfg_.enabled || phc_fd_ < 0)
        return;

    // Only step-correct large offsets; small ones are handled by frequency slew
    if (std::abs(offset_ns) < cfg_.step_threshold_ns)
        return;

    struct timex tx
    {
    };
    tx.modes = ADJ_SETOFFSET | ADJ_NANO;
    tx.time.tv_sec = static_cast<long>(offset_ns / 1'000'000'000LL);
    tx.time.tv_usec = static_cast<long>(offset_ns % 1'000'000'000LL);

    // Handle negative sub-second values
    if (tx.time.tv_usec < 0)
    {
        tx.time.tv_sec -= 1;
        tx.time.tv_usec += 1'000'000'000L;
    }

    (void)phc_clock_adjtime(phc_fd_to_clockid(phc_fd_), &tx);
}

void PhcAdjuster::AdjustFrequency(double rate_ratio)
{
    if (!cfg_.enabled || phc_fd_ < 0)
        return;

    if (!std::isfinite(rate_ratio) || rate_ratio < 0.5 || rate_ratio > 2.0)
        return;

    const double ppb = (rate_ratio - 1.0) * 1e9;
    const double raw_scaled = ppb / 1000.0 * 65536.0;
    constexpr double kMaxScaled = 33'554'432.0;
    const double clamped = raw_scaled < -kMaxScaled ? -kMaxScaled : (raw_scaled > kMaxScaled ? kMaxScaled : raw_scaled);
    const long scaled_ppm = static_cast<long>(clamped);

    struct timex tx
    {
    };
    tx.modes = ADJ_FREQUENCY;
    tx.freq = scaled_ppm;

    (void)phc_clock_adjtime(phc_fd_to_clockid(phc_fd_), &tx);
}

}  // namespace details
}  // namespace ts
}  // namespace score
