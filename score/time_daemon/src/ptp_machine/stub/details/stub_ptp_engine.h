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
#ifndef SCORE_TIME_DAEMON_SRC_PTP_MACHINE_STUB_DETAILS_STUB_PTP_ENGINE_H
#define SCORE_TIME_DAEMON_SRC_PTP_MACHINE_STUB_DETAILS_STUB_PTP_ENGINE_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

#include <chrono>
#include <cstdint>
#include <memory>

namespace score::td::details
{

/**
 * @brief Engine class for interfacing with the libgptp client to manage and retrieve PTP (Precision Time Protocol)
 * data.
 *
 * The StubPTPEngine class encapsulates the logic for initializing, deinitializing, and interacting with the libgptp
 * client. It provides methods to read PTP snapshots, current time values, timebase status, rate deviation, and
 * measurement data related to PDelay and Sync messages. The class also allows querying the time taken to perform a PTP
 * reading.
 */
class StubPTPEngine final
{
  public:
    explicit StubPTPEngine(PtpTimeInfo::ReferenceClock local_clock) noexcept;
    ~StubPTPEngine() noexcept = default;
    auto operator=(const StubPTPEngine&) & noexcept -> StubPTPEngine& = delete;
    auto operator=(StubPTPEngine&&) & noexcept -> StubPTPEngine& = delete;
    StubPTPEngine(const StubPTPEngine&) noexcept = delete;
    StubPTPEngine(StubPTPEngine&&) noexcept = delete;

    /// \brief Method to initialize libgptp client
    ///
    /// \return true - initialize success, otherwise false
    ///
    // Not static: kept as an instance method to match the shape of PTPEngineMockInterface and
    // ShmPTPEngine (the other PTPEngine implementations), even though this particular stub
    // doesn't need instance state — PTPEngine implementations are meant to be interchangeable.
    // (clang-tidy flags this at the definition in the .cpp, not here.)
    [[nodiscard]] auto Initialize() const -> bool;

    /// \brief Method to deinitialize libgptp client
    ///
    /// \return true - deinitialize success, otherwise false
    ///
    [[nodiscard]] auto Deinitialize() const -> bool;

    /// \brief Method that reads PTP snapshot from libgptp
    /// \param info Reference to PtpTimeInfo structure to fill with data
    /// \return true - read success, otherwise false
    ///
    auto ReadPTPSnapshot(PtpTimeInfo& info) -> bool;

    /// \brief Method that calls Libgptp and read current time, timebase status and rate deviation
    ///
    /// \param time_info Reference to PtpTimeInfo structure to fill with data
    ///
    auto ReadTimeValueAndStatus(PtpTimeInfo& time_info) noexcept -> bool;

    /// \brief Method that calls libgptp and read last PDelay ptp data
    ///
    /// \param time_info Reference to PtpTimeInfo structure to fill with PDelay data
    ///
    auto ReadPDelayMeasurementData(PtpTimeInfo& time_info) const noexcept -> bool;

    /// \brief Method that calls libgptp and read last Sync ptp data
    ///
    /// \param time_info Reference to PtpTimeInfo structure to fill with Sync data
    ///
    auto ReadSyncMeasurementData(PtpTimeInfo& time_info) const noexcept -> bool;

  private:
    PtpTimeInfo::ReferenceClock local_clock_;
    std::uint16_t sequence_id_{0U};
};

}  // namespace score::td::details

#endif  // SCORE_TIME_DAEMON_SRC_PTP_MACHINE_STUB_DETAILS_STUB_PTP_ENGINE_H
