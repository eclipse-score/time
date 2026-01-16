/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_CONSUMERTIMEBASEVALIDATIONNOTIFICATIONMOCK_H_
#define SCORE_TIME_CONSUMERTIMEBASEVALIDATIONNOTIFICATIONMOCK_H_

#include <gmock/gmock.h>

#include "score/time/consumer_time_base_validation_notification.h"

namespace score {
namespace time {

class ConsumerTimeBaseValidationNotificationMock final : public ConsumerTimeBaseValidationNotification {
    MOCK_METHOD1(SetPdelayInitiatorData, void(const PdelayInitiatorMeasurementType&));
    MOCK_METHOD1(SetSlaveTimingData, void(const TimeSlaveMeasurementType&));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CONSUMERTIMEBASEVALIDATIONNOTIFICATIONMOCK_H_
