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
#ifndef SCORE_TIME_SLAVE_SRC_GPTP_PHC_PHC_ADJUSTER_H
#define SCORE_TIME_SLAVE_SRC_GPTP_PHC_PHC_ADJUSTER_H

#include <cstdint>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/// Configuration for PHC hardware clock synchronization.
struct PhcConfig
{
    bool enabled = false;
    std::string device = "";                         ///< QNX: "emac0", Linux: "/dev/ptp0"
    std::int64_t step_threshold_ns = 100'000'000LL;  ///< >100ms = step, else slew
};

/**
 * @brief Adjusts the PTP Hardware Clock (PHC) based on gPTP offset and rate.
 *
 * When enabled, applies step corrections for large offsets and frequency
 * slew for continuous tracking.  When disabled, all methods are no-ops.
 *
 * Platform-specific: Linux uses clock_adjtime(), QNX uses EMAC PTP ioctls.
 */
class PhcAdjuster final
{
  public:
    explicit PhcAdjuster(PhcConfig cfg);
    ~PhcAdjuster() noexcept;

    PhcAdjuster(const PhcAdjuster&) = delete;
    PhcAdjuster& operator=(const PhcAdjuster&) = delete;

    /// @return true if hardware clock adjustment is enabled.
    bool IsEnabled() const
    {
        return cfg_.enabled;
    }

    /// Apply a time step or slew based on offset magnitude.
    /// If |offset_ns| > step_threshold_ns, a step correction is applied;
    /// otherwise the offset is ignored (frequency slew handles drift).
    void AdjustOffset(std::int64_t offset_ns);

    /// Adjust the PHC frequency to track the master clock rate.
    /// @param rate_ratio  neighborRateRatio (1.0 = no drift).
    void AdjustFrequency(double rate_ratio);

  private:
    PhcConfig cfg_;
    int phc_fd_{-1};
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_PHC_PHC_ADJUSTER_H
