/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

/// @brief A callback interface for consumer notification

#ifndef SCORE_TIME_CONSUMER_TIME_BASE_VALIDATION_NOTIFICATION_H_
#define SCORE_TIME_CONSUMER_TIME_BASE_VALIDATION_NOTIFICATION_H_

#include "score/time/time_validation_measurement_types.h"

namespace score {
namespace time {

/// @traceid{SWS_TS_00428}
class ConsumerTimeBaseValidationNotification {
public:
    /// @traceid{SWS_TS_01300}
    virtual ~ConsumerTimeBaseValidationNotification() = default;

    /// @param[in] measurementData Detailed timing data for the pDelay Initiator
    /// @traceid{SWS_TS_00422}
    virtual void SetPdelayInitiatorData(const PdelayInitiatorMeasurementType& measurementData) = 0;

    /// @param[in] measurementData Detailed data for validation of the Time Slave
    /// @traceid{SWS_TS_00420}
    virtual void SetSlaveTimingData(const TimeSlaveMeasurementType& measurementData) = 0;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CONSUMER_TIME_BASE_VALIDATION_NOTIFICATION_H_
