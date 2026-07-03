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
#include "score/time_daemon/src/verification_machine/svt/validators/timeout_validator.h"
#include "score/mw/log/logging.h"
#include "score/time_daemon/src/common/logging_contexts.h"

namespace score
{
namespace td
{

TimeoutValidator::TimeoutValidator(PtpTimeInfo::ReferenceClock timeout_clock,
                                   std::chrono::nanoseconds reception_timeout)
    : threshold_{reception_timeout}, timeout_clock_{std::move(timeout_clock)}
{
    reception_time_ = timeout_clock_.Now().TimeSinceEpoch();
}

void TimeoutValidator::DoValidation(PtpTimeInfo& data)
{
    if (IsNewFrameReceived(data))
    {
        // As long as we receive new frames, we update the reception time
        reception_time_ = timeout_clock_.Now().TimeSinceEpoch();

        // reset timeout flag
        data.status.is_timeout = false;
        score::mw::log::LogDebug(kVerificationMachineContext)
            << "TimeoutValidator: New frame received, reset timeout flag";
    }
    else
    {
        // In case no new frame -> check if timeout occurs
        const auto now = timeout_clock_.Now().TimeSinceEpoch();
        const auto now_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
        const auto reception_time_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(reception_time_);
        score::mw::log::LogDebug(kVerificationMachineContext)
            << "TimeoutValidator: received old frame, checking timeout. Now:" << now_nano.count()
            << "ns, Reception time:" << reception_time_nano.count() << "ns";
        if (now_nano > reception_time_nano)
        {
            const auto elapsed = now_nano - reception_time_nano;
            if (elapsed > threshold_)
            {
                // Timeout occurred -> set timeout flag
                data.status.is_timeout = true;
                score::mw::log::LogWarn(kVerificationMachineContext)
                    << "TimeoutValidator: Timeout detected! [" << elapsed.count() << ", " << threshold_.count()
                    << "] ns";
            }
        }
        else
        {
            score::mw::log::LogError(kVerificationMachineContext)
                << "TimeoutValidator: Current time is less than reception time! [Now:" << now_nano.count()
                << "ns, Reception time:" << reception_time_nano.count() << "ns]";
        }
    }
}

bool TimeoutValidator::IsNewFrameReceived(const PtpTimeInfo& data)
{
    // For the initial call, it will always return true, since
    // detection is based on sequence ID, last_received_data_
    // has to be initialized firstly.
    // Then if no new frame is received, seq id will not change
    // so the timeout shall be triggered.
    bool is_new_frame = true;

    if (last_received_data_.has_value())
    {
        // if we have already rx at least one frame
        if (last_received_data_.value().sync_fup_data.sequence_id == data.sync_fup_data.sequence_id)
        {
            // sequence id not changed, so no frame received
            is_new_frame = false;
        }
    }

    last_received_data_ = data;

    return is_new_frame;
}

}  // namespace td
}  // namespace score
