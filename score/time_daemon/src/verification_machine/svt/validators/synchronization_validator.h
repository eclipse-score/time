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
#ifndef SCORE_TIMEDAEMON_CODE_VERIFICATION_MACHINE_SVT_VALIDATORS_SYNCHRONIZATION_VALIDATOR_H
#define SCORE_TIMEDAEMON_CODE_VERIFICATION_MACHINE_SVT_VALIDATORS_SYNCHRONIZATION_VALIDATOR_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/verification_machine/core/verification_stage.h"

namespace score
{
namespace td
{

/**
 * @brief Validation stage that checks synchronization status.
 */
class SynchronizationValidator : public VerificationStage<PtpTimeInfo>
{
  public:
    SynchronizationValidator();

  protected:
    /**
     * @brief Performs synchronization validation.
     */
    void DoValidation(PtpTimeInfo& data) override;

  private:
    bool is_synchronized_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_VERIFICATION_MACHINE_SVT_VALIDATORS_SYNCHRONIZATION_VALIDATOR_H
