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
#ifndef EXAMPLES_TIME_VEHICLE_TIME_VEHICLE_TIME_HANDLER_H
#define EXAMPLES_TIME_VEHICLE_TIME_VEHICLE_TIME_HANDLER_H

#include "score/time/vehicle_time/vehicle_clock.h"
#include "score/time/hpls_time/hpls_clock.h"

#include <cstdint>

namespace examples
{
namespace time
{
namespace vehicle_time
{

/// @brief A snapshot of the combined time report produced by VehicleTimeHandler.
struct TimeReport
{
    /// @brief Current vehicle (PTP-synchronized) time, nanoseconds since VehicleTime epoch.
    std::int64_t vehicle_time_ns{0};

    /// @brief Current local monotonic time from HplsClock, nanoseconds since process boot.
    ///        Provided as a local-time reference alongside the network-synchronized vehicle time.
    std::int64_t hpls_time_ns{0};

    /// @brief True if the vehicle time is currently synchronized to the PTP grand master.
    bool synchronized{false};

    /// @brief True if the vehicle time is in a valid (non-error) state.
    bool valid{false};

    /// @brief Fractional rate deviation of the local clock relative to the grand master.
    ///        Unit: dimensionless (e.g. 1.0e-9 == 1 ppb).  Zero when not synchronized.
    double rate_deviation{0.0};
};

/// @brief Convenience wrapper that reads VehicleClock and HplsClock in one call.
///
/// This class demonstrates how to write a component that depends on two different
/// time bases.  In unit tests, both clocks can be replaced independently via
/// ScopedClockOverride, without any special constructor injection.
///
/// @par Testing pattern
/// @code
///   auto vehicle_mock = std::make_shared<score::time::VehicleClockMock>();
///   auto hpls_mock    = std::make_shared<score::time::HplsClockMock>();
///   score::time::test_utils::ScopedClockOverride<score::time::VehicleTime> vg{vehicle_mock};
///   score::time::test_utils::ScopedClockOverride<score::time::HplsTime>    hg{hpls_mock};
///   EXPECT_CALL(*vehicle_mock, Now()).WillOnce(Return(...));
///   EXPECT_CALL(*hpls_mock,    Now()).WillOnce(Return(...));
///   VehicleTimeHandler handler;
///   const auto report = handler.GetCurrentTime();
/// @endcode
class VehicleTimeHandler
{
  public:
    VehicleTimeHandler()  = default;
    ~VehicleTimeHandler() = default;

    VehicleTimeHandler(const VehicleTimeHandler&)             = delete;
    VehicleTimeHandler& operator=(const VehicleTimeHandler&)  = delete;
    VehicleTimeHandler(VehicleTimeHandler&&)                  = delete;
    VehicleTimeHandler& operator=(VehicleTimeHandler&&)       = delete;

    /// @brief Reads the current vehicle time and local HPLS time and returns a combined report.
    [[nodiscard]] TimeReport GetCurrentTime() const noexcept
    {
        const auto vehicle_snapshot = score::time::VehicleClock::GetInstance().Now();
        const auto hpls_snapshot    = score::time::HplsClock::GetInstance().Now();

        return TimeReport{
            vehicle_snapshot.TimePointNs().count(),
            hpls_snapshot.TimePointNs().count(),
            vehicle_snapshot.Status().IsSynchronized(),
            vehicle_snapshot.Status().IsValid(),
            vehicle_snapshot.Status().rate_deviation,
        };
    }
};

}  // namespace vehicle_time
}  // namespace time
}  // namespace examples

#endif  // EXAMPLES_TIME_VEHICLE_TIME_VEHICLE_TIME_HANDLER_H
