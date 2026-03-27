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
#include "score/TimeSlave/code/gptp/record/recorder.h"

namespace score
{
namespace ts
{
namespace details
{

Recorder::Recorder(Config cfg) : cfg_{std::move(cfg)}
{
    if (cfg_.enabled)
    {
        file_.open(cfg_.file_path, std::ios::out | std::ios::app);
        if (file_.is_open())
        {
            // Write CSV header if the file is empty
            file_.seekp(0, std::ios::end);
            if (file_.tellp() == 0)
            {
                file_ << "mono_ns,event,offset_ns,pdelay_ns,seq_id,status_flags\n";
            }
        }
    }
}

void Recorder::Record(const RecordEntry& entry)
{
    if (!cfg_.enabled || !file_.is_open())
        return;

    std::lock_guard<std::mutex> lk(mutex_);
    file_ << entry.mono_ns << ',' << static_cast<int>(entry.event) << ',' << entry.offset_ns << ',' << entry.pdelay_ns
          << ',' << entry.seq_id << ',' << static_cast<int>(entry.status_flags) << '\n';
    file_.flush();
}

}  // namespace details
}  // namespace ts
}  // namespace score
