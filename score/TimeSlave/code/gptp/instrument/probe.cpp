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
#include "score/TimeSlave/code/gptp/instrument/probe.h"

#include "score/TimeDaemon/code/common/logging_contexts.h"
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
    score::mw::log::LogDebug(score::td::kGPtpMachineContext)
        << "PROBE point=" << static_cast<int>(point)
        << " ts=" << data.ts_mono_ns
        << " val=" << data.value_ns
        << " seq=" << data.seq_id;

    if (recorder_ != nullptr && recorder_->IsEnabled())
    {
        recorder_->Record(RecordEntry{
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
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

}  // namespace details
}  // namespace ts
}  // namespace score
