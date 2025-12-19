/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_TIME_PRECISION_MEASUREMENT_TYPES_H_
#define SCORE_TIME_TIME_PRECISION_MEASUREMENT_TYPES_H_

#include <cstdint>

namespace score {
namespace time {

/// @traceid{SWS_TS_01400}
struct TimePrecisionMeasurement final {
    /// @traceid{SWS_TS_01401}
    std::uint32_t glbSeconds{};

    /// @traceid{SWS_TS_01402}
    std::uint32_t glbNanoSeconds{};

    /// @traceid{SWS_TS_01403}
    std::uint8_t timeBaseStatus{};

    /// @traceid{SWS_TS_01404}
    std::uint32_t virtualLocalTimeLow{};

    /// @traceid{SWS_TS_01405}
    std::int16_t rateDeviation{};

    /// @traceid{SWS_TS_01406}
    std::uint32_t locSeconds{};

    /// @traceid{SWS_TS_01407}
    std::uint32_t locNanoSeconds{};

    /// @traceid{SWS_TS_01408}
    std::uint32_t pathDelay{};
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_TIME_PRECISION_MEASUREMENT_TYPES_H_
