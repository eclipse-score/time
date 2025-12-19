/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_CONSUMER_MOCK_H
#define SCORE_PTP_CONSUMER_MOCK_H

#include "inc/score_ptp_consumer.h"
#include <gmock/gmock.h>

class PtpConsumerMock {
public:
    // Mock method for TSync_GetCurrentVirtualLocalTime
    MOCK_METHOD(TSync_ReturnType, TSync_GetCurrentVirtualLocalTime,
                (TSync_TimeBaseHandleType timeBaseHandle,
                TSync_VirtualLocalTimeType* localTimePtr));

    // Mock method for TSync_BusSetGlobalTime
    MOCK_METHOD(TSync_ReturnType, TSync_BusSetGlobalTime,
                (TSync_TimeBaseHandleType timeBaseHandle,
                const TSync_TimeStampType* globalTimePtr,
                const TSync_UserDataType* userDataPtr,
                const TSync_MeasurementType* measureDataPtr,
                const TSync_VirtualLocalTimeType* localTimePtr));

    // Mock method for Score_PtpReadSubTlvFromBuffer
    MOCK_METHOD(Score_PtpBooleanType, Score_PtpReadSubTlvFromBuffer,
                (const uint8_t *autosarTlvBufferIn));
};

#endif /* SCORE_PTP_CONSUMER_MOCK_H */
