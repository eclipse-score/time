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
#ifndef SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_VALIDATORS_TIMEOUT_VALIDATOR_H
#define SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_VALIDATORS_TIMEOUT_VALIDATOR_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/verification_machine/core/verification_stage.h"

#include <optional>

namespace score
{
namespace td
{

/**
 * @brief Validation stage that checks if PTP frames are received, and raise TIMEOUT flag accordingly
 * @param timeout_clock - reference clock object - used to calculate timeout
 * @param reception_timeout - threshold in nanoseconds to define max timeout threshold
 */
class TimeoutValidator : public VerificationStage<PtpTimeInfo>
{
  public:
    explicit TimeoutValidator(PtpTimeInfo::ReferenceClock timeout_clock,
                              std::chrono::nanoseconds reception_timeout);
    virtual ~TimeoutValidator() = default;

  protected:
    void DoValidation(PtpTimeInfo& data) override;

  private:
    bool IsNewFrameReceived(const PtpTimeInfo& data);

    const std::chrono::nanoseconds threshold_;
    PtpTimeInfo::ReferenceClock timeout_clock_;
    PtpTimeInfo::ReferenceClock::duration reception_time_;
    std::optional<PtpTimeInfo> last_received_data_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_SVT_VALIDATORS_TIMEOUT_VALIDATOR_H
