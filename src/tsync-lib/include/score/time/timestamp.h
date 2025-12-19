/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_TIMESTAMP_H_
#define SCORE_TIME_TIMESTAMP_H_

#include <chrono>
#include <cstdint>

namespace score {
namespace time {

/// @traceid{SWS_TS_01260}
struct TimeBase {
    /// @traceid{SWS_TS_01261}
    using rep = std::int64_t;
    /// @traceid{SWS_TS_01262}
    using period = std::nano;
    /// @traceid{SWS_TS_01263}
    using duration = std::chrono::duration<rep, period>;
    /// @traceid{SWS_TS_01264}
    using time_point = std::chrono::time_point<TimeBase>;
    /// @traceid{SWS_TS_01265}
    constexpr static bool is_steady = false;
};

/// @traceid{SWS_TS_01251}
using Timestamp = std::chrono::time_point<TimeBase, std::chrono::nanoseconds>;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_TIMESTAMP_H_
