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
#ifndef SCORE_TIME_DAEMON_SRC_PTP_MACHINE_SHM_DETAILS_SHM_PTP_ENGINE_H
#define SCORE_TIME_DAEMON_SRC_PTP_MACHINE_SHM_DETAILS_SHM_PTP_ENGINE_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/ts_client/src/gptp_ipc_receiver.h"

#include <string>

namespace score::td::details
{

/**
 * @brief PTP engine that reads time data from the shared-memory IPC channel
 *        written by TimeSlave via GptpIpcPublisher.
 *
 * Converts the ts_client/src-internal GptpIpcData to the TimeDaemon PtpTimeInfo
 * data model.
 */
class ShmPTPEngine final
{
  public:
    // kGptpIpcName is a char-array constant used as a default arg for a std::string param;
    // ordinary literal decay, not raw pointer/buffer use.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    explicit ShmPTPEngine(std::string ipc_name = score::ts::details::kGptpIpcName) noexcept;
    ~ShmPTPEngine() noexcept = default;

    ShmPTPEngine(const ShmPTPEngine&) = delete;
    auto operator=(const ShmPTPEngine&) -> ShmPTPEngine& = delete;
    ShmPTPEngine(ShmPTPEngine&&) = delete;
    auto operator=(ShmPTPEngine&&) -> ShmPTPEngine& = delete;

    auto Initialize() -> bool;

    auto Deinitialize() -> bool;

    auto ReadPTPSnapshot(PtpTimeInfo& info) -> bool;

  private:
    std::string ipc_name_;
    score::ts::details::GptpIpcReceiver receiver_;
    bool initialized_{false};
};

}  // namespace score::td::details

#endif  // SCORE_TIME_DAEMON_SRC_PTP_MACHINE_SHM_DETAILS_SHM_PTP_ENGINE_H
