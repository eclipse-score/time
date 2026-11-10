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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_SCORE_VEHICLE_TIME_SCORE_VEHICLE_TIMEBASE_HANDLER_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_SCORE_VEHICLE_TIME_SCORE_VEHICLE_TIMEBASE_HANDLER_H

#include "score/time_daemon/src/application/timebase_handlers/generic/generic_timebase_handler.h"
#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

namespace score
{
namespace td
{

/// \brief S-CORE-specific implementation of a TimebaseHandler for Vehicle Time.
///
/// The ScoreVehicleTimebaseHandler class manages the initialization, execution, and stopping
/// of a Vehicle Time timebase. It integrates with various
/// subsystems like the GPTP machine, verification machine, IPC publisher,
/// and control flow divider to provide a fully functional timebase handler.
///
/// This class is non-copyable and non-movable to ensure proper resource
/// management.
class ScoreVehicleTimebaseHandler final : public GenericTimebaseHandler<PtpTimeInfo>
{
  public:
    ScoreVehicleTimebaseHandler() noexcept;
    virtual ~ScoreVehicleTimebaseHandler() noexcept = default;
    ScoreVehicleTimebaseHandler(const ScoreVehicleTimebaseHandler&) = delete;
    ScoreVehicleTimebaseHandler(ScoreVehicleTimebaseHandler&&) = delete;
    ScoreVehicleTimebaseHandler& operator=(const ScoreVehicleTimebaseHandler&) = delete;
    ScoreVehicleTimebaseHandler& operator=(ScoreVehicleTimebaseHandler&&) = delete;

  private:
    void CreateComponents() noexcept override;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_SCORE_VEHICLE_TIME_SCORE_VEHICLE_TIMEBASE_HANDLER_H
