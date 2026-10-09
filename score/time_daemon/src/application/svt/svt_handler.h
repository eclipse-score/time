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

#include "score/time_daemon/src/application/generic/generic_timebase_handler.h"
#include "score/time_daemon/src/application/timebase_handler.h"
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

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
    GenericTimebaseHandler<PtpTimeInfo> handler_;  ///< Owns the SVT machines and their topic wiring
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_HANDLER_H
