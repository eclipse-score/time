/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

/// @brief A callback interface for validator notification

#ifndef SCORE_TIME_PROVIDER_TIME_BASE_VALIDATION_NOTIFICATION_H_
#define SCORE_TIME_PROVIDER_TIME_BASE_VALIDATION_NOTIFICATION_H_

#include "score/time/time_validation_measurement_types.h"

namespace score {
namespace time {

/// @traceid{SWS_TS_00419}
class ProviderTimeBaseValidationNotification {
public:
    /// @traceid{SWS_TS_01301}
    virtual ~ProviderTimeBaseValidationNotification() = default;

    /// @param[in] measurementData Detailed timing data for the pDelay Responder
    /// @traceid{SWS_TS_00423}
    virtual void SetPdelayResponderData(const PdelayResponderMeasurementType& measurementData) = 0;

    /// @param[in] measurementData Detailed data for validation of the Time Master
    /// @traceid{SWS_TS_00421}
    virtual void SetMasterTimingData(const TimeMasterMeasurementType& measurementData) = 0;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_PROVIDER_TIME_BASE_VALIDATION_NOTIFICATION_H_
