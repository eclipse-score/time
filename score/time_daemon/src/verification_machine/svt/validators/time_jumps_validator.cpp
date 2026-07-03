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
#include "score/time_daemon/src/verification_machine/svt/validators/time_jumps_validator.h"
#include "score/mw/log/logging.h"
#include "score/time_daemon/src/common/logging_contexts.h"

namespace score
{
namespace td
{

TimeJumpsValidator::TimeJumpsValidator(PtpTimeInfo::ReferenceClock debouncing_clock,
                                       std::chrono::nanoseconds max_time_jump_allowed,
                                       std::chrono::nanoseconds sync_debounce_threshold,
                                       std::uint8_t valid_frames_threshold)
    : max_time_jump_allowed_{max_time_jump_allowed},
      sync_debounce_threshold_{sync_debounce_threshold},
      valid_frames_threshold_{valid_frames_threshold},
      time_jump_state_{TimeJumpState::kNoTimeJump},
      current_state_{ProcessingStates::kIdle},
      last_sync_frame_{std::nullopt},
      sync_debouncing_init_time_{std::chrono::nanoseconds::zero()},
      debouncing_clock_{std::move(debouncing_clock)},
      valid_frames_cnt_{0U}
{
}

void TimeJumpsValidator::DoValidation(PtpTimeInfo& data)
{
    switch (current_state_)
    {
        case ProcessingStates::kIdle:
            HandleIdleState(data);
            break;
        case ProcessingStates::kInitialSyncDebouncing:
            HandleInitialSyncDebouncingState();
            break;
        case ProcessingStates::kTimeJumpHandling:
            SyncFramesHandler(data);
            break;
        default:
            break;
    }
}

bool TimeJumpsValidator::IsTimeJumpDetected(const PtpTimeInfo& data)
{
    bool is_time_jump_detected{false};

    if (data.sync_fup_data.sync_ingress_timestamp > last_sync_frame_.value().sync_fup_data.sync_ingress_timestamp)
    {
        // calculate t2 rx timestamp diff, between last and current frames
        const auto t2_diff =
            data.sync_fup_data.sync_ingress_timestamp - last_sync_frame_.value().sync_fup_data.sync_ingress_timestamp;
        // calculate predicted GM preciseOriginTimestamp based on t2 diff between last received frame and current
        const auto predicted_origin_timestamp =
            last_sync_frame_.value().sync_fup_data.precise_origin_timestamp + t2_diff;

        if (predicted_origin_timestamp > data.sync_fup_data.precise_origin_timestamp)
        {
            const auto current_jump_to_past = predicted_origin_timestamp - data.sync_fup_data.precise_origin_timestamp;

            // if the difference is higher than max allowed threshold -> time jump to past detected
            if (current_jump_to_past > static_cast<std::uint64_t>(max_time_jump_allowed_.count()))
            {
                is_time_jump_detected = true;
                time_jump_state_ = TimeJumpState::kJumpToPast;
                score::mw::log::LogWarn(kVerificationMachineContext)
                    << "TimeJumpsValidator: Time jump to past detected! Jump = " << current_jump_to_past << " ns";
            }
        }
        else if (predicted_origin_timestamp < data.sync_fup_data.precise_origin_timestamp)
        {
            const auto current_jump_to_future =
                data.sync_fup_data.precise_origin_timestamp - predicted_origin_timestamp;

            if (current_jump_to_future > static_cast<std::uint64_t>(max_time_jump_allowed_.count()))
            {
                is_time_jump_detected = true;
                time_jump_state_ = TimeJumpState::kJumpToFuture;
                score::mw::log::LogWarn(kVerificationMachineContext)
                    << "TimeJumpsValidator: Time jump to future detected! Jump = " << current_jump_to_future << " ns";
            }
        }
        else
        {
            // No time-jump
        }
    }
    else
    {
        score::mw::log::LogError(kVerificationMachineContext)
            << "TimeJumpsValidator: Current Sync T2 is less than previous: [Current Sync: " << data.sync_fup_data
            << "], [Prv Sync: " << last_sync_frame_.value().sync_fup_data << "]";
    }

    return is_time_jump_detected;
}

void TimeJumpsValidator::HandleIdleState(const PtpTimeInfo& data)
{
    if (data.status.is_synchronized)
    {
        GoToInitialSyncDebouncing();
    }
    else
    {
        score::mw::log::LogDebug(kVerificationMachineContext)
            << "TimeJumpsValidator: Waiting until status.is_synchronized becomes true. Remain in kIdle state";
    }
}

void TimeJumpsValidator::HandleInitialSyncDebouncingState()
{
    if ((sync_debouncing_init_time_ + sync_debounce_threshold_) < debouncing_clock_.Now().TimeSinceEpoch())
    {
        GoToTimeJumpHandling();
    }
}

void TimeJumpsValidator::SyncFramesHandler(PtpTimeInfo& data)
{
    if (last_sync_frame_.has_value())
    {
        if (not(IsTimeJumpDetected(data)))
        {
            valid_frames_cnt_++;
            if (valid_frames_cnt_ >= valid_frames_threshold_)
            {
                // Clear timejump
                time_jump_state_ = TimeJumpState::kNoTimeJump;
            }
        }
        else
        {
            valid_frames_cnt_ = 0U;
        }
    }

    last_sync_frame_ = data;

    UpdateStatus(data);
}

void TimeJumpsValidator::UpdateStatus(PtpTimeInfo& data)
{
    // Update Time jump flags
    switch (time_jump_state_)
    {
        case TimeJumpState::kJumpToFuture:
            data.status.is_time_jump_future = true;
            data.status.is_time_jump_past = false;
            break;
        case TimeJumpState::kJumpToPast:
            data.status.is_time_jump_future = false;
            data.status.is_time_jump_past = true;
            break;
        case TimeJumpState::kNoTimeJump:
            data.status.is_time_jump_future = false;
            data.status.is_time_jump_past = false;
            break;
        default:
            break;
    }
}

void TimeJumpsValidator::GoToInitialSyncDebouncing()
{
    // Set debouncing timer, so we will calculate sync debouncing time from this point
    sync_debouncing_init_time_ = debouncing_clock_.Now().TimeSinceEpoch();
    current_state_ = ProcessingStates::kInitialSyncDebouncing;
    score::mw::log::LogDebug(kVerificationMachineContext)
        << "TimeJumpsValidator: Switch to kInitialSyncDebouncing state";
}

void TimeJumpsValidator::GoToTimeJumpHandling()
{
    current_state_ = ProcessingStates::kTimeJumpHandling;
    score::mw::log::LogDebug(kVerificationMachineContext) << "TimeJumpsValidator: Switch to kTimeJumpHandling state";
}

}  // namespace td
}  // namespace score
