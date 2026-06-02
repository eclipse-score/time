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

#include <cmath>

// Extern C functions from qnx_raw_shim.cpp
extern "C" int qnx_phc_open(const char* phc_dev);
extern "C" int qnx_phc_adjtime_step(int phc_fd, long long offset_ns);
extern "C" int qnx_phc_adjfreq_ppb(int phc_fd, long long freq_ppb);

namespace score
{
namespace ts
{
namespace details
{

PhcAdjuster::PhcAdjuster(PhcConfig cfg) : cfg_{std::move(cfg)}
{
    if (cfg_.enabled && !cfg_.device.empty())
    {
        phc_fd_ = qnx_phc_open(cfg_.device.c_str());
    }
}

PhcAdjuster::~PhcAdjuster()
{
    phc_fd_ = -1;
}

void PhcAdjuster::AdjustOffset(std::int64_t offset_ns)
{
    if (!cfg_.enabled)
        return;

    // Only step-correct large offsets; small ones are handled by frequency slew
    if (std::abs(offset_ns) < cfg_.step_threshold_ns)
        return;

    (void)qnx_phc_adjtime_step(phc_fd_, static_cast<long long>(offset_ns));
}

void PhcAdjuster::AdjustFrequency(double rate_ratio)
{
    if (!cfg_.enabled)
        return;

    // Convert rate_ratio to ppb offset from 1.0
    // rate_ratio = slave_interval / master_interval
    // ppb = (rate_ratio - 1.0) * 1e9
    const auto ppb = static_cast<long long>((rate_ratio - 1.0) * 1e9);

    (void)qnx_phc_adjfreq_ppb(phc_fd_, ppb);
}

}  // namespace details
}  // namespace ts
}  // namespace score
