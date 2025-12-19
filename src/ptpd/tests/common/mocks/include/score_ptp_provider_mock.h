/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_PROVIDER_MOCK_H
#define SCORE_PTP_PROVIDER_MOCK_H

#include "inc/score_ptp_common.h"
#include <gmock/gmock.h>

class PtpProviderMock {
public:
    // Mock method for TSync_GetCurrentVirtualLocalTime
    MOCK_METHOD(TSync_ReturnType, TSync_GetCurrentVirtualLocalTime,
                (TSync_TimeBaseHandleType timeBaseHandle,
                TSync_VirtualLocalTimeType* localTimePtr));

    // Mock method for TSync_BusGetGlobalTime
    MOCK_METHOD(TSync_ReturnType, TSync_BusGetGlobalTime,
                (TSync_TimeBaseHandleType timeBaseHandle,
                TSync_TimeStampType* globalTimePtr,
                TSync_UserDataType* userDataPtr,
                TSync_MeasurementType* measureDataPtr,
                TSync_VirtualLocalTimeType* localTimePtr));

};

#endif /* SCORE_PTP_PROVIDER_MOCK_H */
