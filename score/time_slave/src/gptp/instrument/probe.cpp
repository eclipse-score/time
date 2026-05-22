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
#include "score/time_slave/src/gptp/instrument/probe.h"

#include "score/time_slave/src/common/logging_contexts.h"
#include "score/mw/log/logging.h"

#include <time.h>

namespace score
{
namespace ts
{
namespace details
{

ProbeManager& ProbeManager::Instance()
{
    static ProbeManager instance;
    return instance;
}

void ProbeManager::Trace(ProbePoint point, const ProbeData& data)
{
    score::mw::log::LogDebug(score::ts::kGPtpMachineContext)
        << "PROBE point=" << static_cast<int>(point) << " ts=" << data.ts_mono_ns << " val=" << data.value_ns
        << " seq=" << data.seq_id;

    Recorder* const rec = recorder_.load(std::memory_order_acquire);
    if (rec != nullptr && rec->IsEnabled())
    {
        rec->Record(RecordEntry{
            data.ts_mono_ns,
            RecordEvent::kProbe,
            data.value_ns,
            0,
            static_cast<std::uint16_t>(data.seq_id),
            static_cast<std::uint8_t>(point),
        });
    }
}

std::int64_t ProbeMonoNs() noexcept
{
    ::timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL + static_cast<std::int64_t>(ts.tv_nsec);
}

}  // namespace details
}  // namespace ts
}  // namespace score
