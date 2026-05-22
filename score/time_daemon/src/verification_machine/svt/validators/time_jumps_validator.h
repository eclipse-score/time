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
#ifndef SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_VALIDATORS_TIME_JUMPS_VALIDATOR_H
#define SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_VALIDATORS_TIME_JUMPS_VALIDATOR_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/verification_machine/core/verification_stage.h"

#include <chrono>
#include <optional>

namespace score
{
namespace td
{

/**
 * @brief Validation stage that checks if PTP master is stable by checking if time-jump occurred.
 * @param debouncing_clock - reference clock object - used to calculate time between when SYNC flag is set to the moment
 * when time jump validation is enabled
 * @param max_time_jump_allowed - threshold in nanoseconds to define max allowed time jump value
 * @param sync_debounce_threshold - threshold in nanoseconds to define max sync debounce time
 * @param valid_frames_threshold - threshold to define number of valid packages to receive to swith from time jump state
 * to normal
 */
class TimeJumpsValidator : public VerificationStage<PtpTimeInfo>
{
  public:
    TimeJumpsValidator(std::unique_ptr<PtpTimeInfo::ReferenceClock> debouncing_clock,
                       std::chrono::nanoseconds max_time_jump_allowed,
                       std::chrono::nanoseconds sync_debounce_threshold,
                       std::uint8_t valid_frames_threshold);

  protected:
    void DoValidation(PtpTimeInfo& data) override;

  private:
    void HandleIdleState(const PtpTimeInfo& data);
    void HandleInitialSyncDebouncingState();
    void SyncFramesHandler(PtpTimeInfo& data);
    bool IsTimeJumpDetected(const PtpTimeInfo& data);
    void UpdateStatus(PtpTimeInfo& data);
    void GoToInitialSyncDebouncing();
    void GoToTimeJumpHandling();

    enum class ProcessingStates : std::uint8_t
    {
        kIdle,
        kInitialSyncDebouncing,
        kTimeJumpHandling
    };

    enum class TimeJumpState : std::uint8_t
    {
        kJumpToFuture,
        kJumpToPast,
        kNoTimeJump
    };

    const std::chrono::nanoseconds max_time_jump_allowed_;
    const std::chrono::nanoseconds sync_debounce_threshold_;
    const std::uint8_t valid_frames_threshold_;
    TimeJumpState time_jump_state_;
    ProcessingStates current_state_;
    std::optional<PtpTimeInfo> last_sync_frame_;
    PtpTimeInfo::ReferenceClock::duration sync_debouncing_init_time_;
    std::unique_ptr<PtpTimeInfo::ReferenceClock> debouncing_clock_;
    std::uint8_t valid_frames_cnt_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_VALIDATORS_TIME_JUMPS_VALIDATOR_H
