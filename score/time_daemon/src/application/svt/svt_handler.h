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
class SvtHandler final : public GenericTimebaseHandler<PtpTimeInfo>
{
  public:
    SvtHandler() noexcept;
    virtual ~SvtHandler() noexcept = default;
    SvtHandler(const SvtHandler&) = delete;
    SvtHandler(SvtHandler&&) = delete;
    SvtHandler& operator=(const SvtHandler&) = delete;
    SvtHandler& operator=(SvtHandler&&) = delete;

  private:
    void CreateComponents() noexcept override;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_SVT_HANDLER_H
