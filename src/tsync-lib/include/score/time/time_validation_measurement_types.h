/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_TIME_VALIDATION_MEASUREMENT_TYPES_H_
#define SCORE_TIME_TIME_VALIDATION_MEASUREMENT_TYPES_H_

#include <cstdint>

#include "score/time/timestamp.h"

namespace score {
namespace time {

/// @traceid{SWS_TS_00414}
struct TimeMasterMeasurementType final {
    /// @traceid{SWS_TS_14140}
    score::time::Timestamp preciseOriginTimestamp{};

    /// @traceid{SWS_TS_14141}
    std::uint64_t syncEgressTimestamp{};

    /// @traceid{SWS_TS_14142}
    std::uint16_t sequenceId{};
};

/// @traceid{SWS_TS_00415}
struct TimeSlaveMeasurementType final {
    /// @traceid{SWS_TS_14150}
    score::time::Timestamp preciseOriginTimestamp{};

    /// @traceid{SWS_TS_14151}
    score::time::Timestamp referenceGlobalTimestamp{};

    /// @traceid{SWS_TS_14152}
    std::uint64_t syncIngressTimestamp{};

    /// @traceid{SWS_TS_14153}
    std::int64_t correctionField{};

    /// @traceid{SWS_TS_14154}
    std::uint64_t referenceLocalTimestamp{};

    /// @traceid{SWS_TS_14155}
    std::uint32_t pDelay{};

    /// @traceid{SWS_TS_14156}
    std::uint16_t sequenceId{};
};

/// @traceid{SWS_TS_00416}
struct PdelayInitiatorMeasurementType final {
    /// @traceid{SWS_TS_14160}
    std::uint64_t requestOriginTimestamp{};

    /// @traceid{SWS_TS_14161}
    std::uint64_t responseReceiptTimestamp{};

    /// @traceid{SWS_TS_14162}
    score::time::Timestamp requestReceiptTimestamp{};

    /// @traceid{SWS_TS_14163}
    score::time::Timestamp responseOriginTimestamp{};

    /// @traceid{SWS_TS_14164}
    std::uint64_t referenceLocalTimestamp{};

    /// @traceid{SWS_TS_14165}
    score::time::Timestamp referenceGlobalTimestamp{};

    /// @traceid{SWS_TS_14166}
    std::uint32_t pDelay{};

    /// @traceid{SWS_TS_14167}
    std::uint16_t sequenceId{};
};

/// @traceid{SWS_TS_00417}
struct PdelayResponderMeasurementType final {
    /// @traceid{SWS_TS_14170}
    std::uint64_t requestReceiptTimestamp{};

    /// @traceid{SWS_TS_14171}
    std::uint64_t responseOriginTimestamp{};

    /// @traceid{SWS_TS_14172}
    std::uint64_t referenceLocalTimestamp{};

    /// @traceid{SWS_TS_14173}
    score::time::Timestamp referenceGlobalTimestamp{};

    /// @traceid{SWS_TS_14174}
    std::uint16_t sequenceId{};
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_TIME_VALIDATION_MEASUREMENT_TYPES_H_
