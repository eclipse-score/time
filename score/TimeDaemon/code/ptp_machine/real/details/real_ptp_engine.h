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
#ifndef SCORE_TIMEDAEMON_CODE_PTP_MACHINE_REAL_DETAILS_REAL_PTP_ENGINE_H
#define SCORE_TIMEDAEMON_CODE_PTP_MACHINE_REAL_DETAILS_REAL_PTP_ENGINE_H

#include "score/TimeDaemon/code/common/data_types/ptp_time_info.h"
#include "score/libTSClient/gptp_ipc_receiver.h"

#include <string>

namespace score
{
namespace td
{
namespace details
{

/**
 * @brief PTP engine that reads time data from the IPC channel written by TimeSlave.
 */
class RealPTPEngine final
{
  public:
    explicit RealPTPEngine(std::string ipc_name = score::ts::details::kGptpIpcName) noexcept;
    ~RealPTPEngine() noexcept = default;

    RealPTPEngine(const RealPTPEngine&) = delete;
    RealPTPEngine& operator=(const RealPTPEngine&) = delete;
    RealPTPEngine(RealPTPEngine&&) = delete;
    RealPTPEngine& operator=(RealPTPEngine&&) = delete;

    /// Open and map the IPC channel.
    /// @return true on success.
    bool Initialize();

    /// Unmap the IPC channel.
    /// @return true (always succeeds).
    bool Deinitialize();

    /// Read a fresh snapshot from the IPC channel and populate @p info.
    /// Delegates to ReadTimeValueAndStatus, ReadPDelayMeasurementData,
    /// and ReadSyncMeasurementData.
    bool ReadPTPSnapshot(PtpTimeInfo& info);

    bool ReadTimeValueAndStatus(PtpTimeInfo& info) noexcept;
    bool ReadPDelayMeasurementData(PtpTimeInfo& info) const noexcept;
    bool ReadSyncMeasurementData(PtpTimeInfo& info) const noexcept;

  private:
    std::string ipc_name_;
    score::ts::details::GptpIpcReceiver receiver_;
    bool initialized_{false};
    PtpTimeInfo cached_{};
};

}  // namespace details
}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_PTP_MACHINE_REAL_DETAILS_REAL_PTP_ENGINE_H
