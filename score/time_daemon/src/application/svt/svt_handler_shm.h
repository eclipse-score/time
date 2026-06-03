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
#ifndef SCORE_TIMEDAEMON_CODE_APPLICATION_SVT_SHM_HANDLER_H
#define SCORE_TIMEDAEMON_CODE_APPLICATION_SVT_SHM_HANDLER_H

#include "score/TimeDaemon/code/application/job_runner/job_runner.h"
#include "score/TimeDaemon/code/application/timebase_handler.h"
#include "score/TimeDaemon/code/control_flow_divider/ptp/ptp_control_flow_divider.h"
#include "score/TimeDaemon/code/ipc/svt/publisher/svt_publisher.h"
#include "score/TimeDaemon/code/msg_broker/msg_broker.h"
#include "score/TimeDaemon/code/ptp_machine/shm/gptp_shm_machine.h"
#include "score/TimeDaemon/code/verification_machine/svt/svt_verification_machine.h"

#include <memory>

namespace score
{
namespace td
{

/// \brief SVT timebase handler backed by the shared-memory gPTP engine.
///
/// Reads live gPTP data written by TimeSlave via GptpIpcPublisher, processing
/// it through the same verification and IPC-publish pipeline as SvtHandler.
class SvtHandlerShm : public TimebaseHandler
{
  public:
    SvtHandlerShm() noexcept;
    virtual ~SvtHandlerShm() noexcept = default;
    SvtHandlerShm(const SvtHandlerShm&) = delete;
    SvtHandlerShm(SvtHandlerShm&&) = delete;
    SvtHandlerShm& operator=(const SvtHandlerShm&) = delete;
    SvtHandlerShm& operator=(SvtHandlerShm&&) = delete;

    virtual void Initialize() noexcept override;
    virtual void RunOnce(const score::cpp::stop_token& token) noexcept override;
    virtual void Stop() noexcept override;

  private:
    std::unique_ptr<JobRunner> job_runner_;
    std::shared_ptr<MessageBroker<PtpTimeInfo>> msg_broker_;
    std::shared_ptr<GPTPShmMachine> gptp_machine_;
    std::shared_ptr<SvtVerificationMachine> verification_machine_;
    std::shared_ptr<SvtPublisher> ipc_publisher_;
    std::shared_ptr<PtpControlFlowDivider> ctrl_flow_divider_;
    TimebaseHandler::Status handler_status_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_APPLICATION_SVT_SHM_HANDLER_H
