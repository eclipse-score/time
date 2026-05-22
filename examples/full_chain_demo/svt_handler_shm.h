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

// ============================================================================
// BACKUP FILE — not compiled from this location.
//
// Original location: score/time_daemon/src/application/svt/svt_handler_shm.h
//
// To use demo_time_daemon, these 4 files must be copied back to
//   score/time_daemon/src/application/svt/
// and the svt_handler_shm target added to that package's BUILD.
// All deps have visibility restricted to //score/time_daemon:__subpackages__,
// so the BUILD target cannot live in //examples.
//
// See examples/full_chain_demo/BUILD for the commented-out cc_binary entry.
// ============================================================================

#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_SHM_HANDLER_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_SHM_HANDLER_H

#include "score/time_daemon/src/application/job_runner/job_runner.h"
#include "score/time_daemon/src/application/timebase_handler.h"
#include "score/time_daemon/src/control_flow_divider/ptp/ptp_control_flow_divider.h"
#include "score/time_daemon/src/ipc/svt/publisher/svt_publisher.h"
#include "score/time_daemon/src/msg_broker/msg_broker.h"
#include "score/time_daemon/src/ptp_machine/shm/gptp_shm_machine.h"
#include "score/time_daemon/src/verification_machine/svt/svt_verification_machine.h"

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

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_SHM_HANDLER_H
