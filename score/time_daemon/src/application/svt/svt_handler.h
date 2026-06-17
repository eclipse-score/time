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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_HANDLER_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_HANDLER_H

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

/// \brief Concrete implementation of a TimebaseHandler for SVT.
///
/// The SvtHandler class manages the initialization, execution, and stopping
/// of a SVT (Synchronous Vehicle Time) timebase. It integrates with various
/// subsystems like the GPTP machine, verification machine, IPC publisher,
/// and control flow divider to provide a fully functional timebase handler.
///
/// This class is non-copyable and non-movable to ensure proper resource
/// management.
class SvtHandler : public TimebaseHandler
{
  public:
    SvtHandler() noexcept;
    virtual ~SvtHandler() noexcept = default;
    SvtHandler(const SvtHandler&) = delete;
    SvtHandler(SvtHandler&&) = delete;
    SvtHandler& operator=(const SvtHandler&) = delete;
    SvtHandler& operator=(SvtHandler&&) = delete;

    /// \brief Initializes the SVT timebase handler
    ///
    /// This function sets up all necessary subsystems and prepares the handler
    /// for running. It overrides the abstract Initialize method from
    /// TimebaseHandler.
    virtual void Initialize() noexcept override;

    /// \brief Runs once the SVT timebase handler main functionality
    ///
    /// This function handles the async. initialization and starting the main functionality,
    /// based on init result. It shall be called periodically from main thread, as the
    /// operation is non blocking
    ///
    /// \param token Stop token used to safely terminate the run loop
    virtual void RunOnce(const score::cpp::stop_token& token) noexcept override;

    /// \brief Stops the SVT timebase handler
    ///
    /// Safely stops the timebase operations and releases any resources.
    /// Overrides the abstract Stop method from TimebaseHandler.
    virtual void Stop() noexcept override;

  private:
    std::unique_ptr<JobRunner> job_runner_;                         ///< Manages periodic jobs and tasks
    std::shared_ptr<MessageBroker<PtpTimeInfo>> msg_broker_;        ///< Handles message communication
    std::shared_ptr<GPTPShmMachine> gptp_machine_;                  ///< Manages GPTP synchronization
    std::shared_ptr<SvtVerificationMachine> verification_machine_;  ///< Handles SVT verification
    std::shared_ptr<SvtPublisher> ipc_publisher_;                   ///< Publishes SVT data via IPC
    std::shared_ptr<PtpControlFlowDivider> ctrl_flow_divider_;      ///< Divides PTP control flow
    TimebaseHandler::Status handler_status_;                        ///< Current status of the handler
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_HANDLER_H
