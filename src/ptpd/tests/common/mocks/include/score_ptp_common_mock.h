/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_COMMON_MOCK_H
#define SCORE_PTP_COMMON_MOCK_H

#include <gmock/gmock.h>

#include "inc/score_ptp_common.h"

#define CLOCK_IDENTITY_LENGTH 8

class PtpCommonMock {
   public:
    // Mock method for tsync_ptp_lib functions
    MOCK_METHOD(TSync_ReturnType, TSync_Open, ());
    MOCK_METHOD(TSync_TimeBaseHandleType, TSync_OpenTimebase, (TSync_SynchronizedTimeBaseType));
    MOCK_METHOD(TSync_ReturnType, TSync_RegisterTransmitGlobalTimeCallback,
                (TSync_TimeBaseHandleType timeBaseHandle, TSync_TransmitGlobalTimeCallback cb));
    MOCK_METHOD(void, TSync_Close, ());
    MOCK_METHOD(void, TSync_CloseTimebase, (TSync_TimeBaseHandleType timeBaseHandle));
    MOCK_METHOD(TSync_ReturnType, Score_PtpTransmitGlobalTimeCallback, (TSync_SynchronizedTimeBaseType domainId));
    MOCK_METHOD(TSync_ReturnType, TSync_GetTimebaseConfiguration,
                (TSync_TimeBaseHandleType timeBaseHandle, TSync_Role role_requested,
                 TSync_TimeBaseConfiguration* config));
};

extern PtpCommonMock ptpCommonMock;

// Define stub functions
TSync_ReturnType TSync_Open(void) {
    return ptpCommonMock.TSync_Open();
}

TSync_ReturnType TSync_RegisterTransmitGlobalTimeCallback(TSync_TimeBaseHandleType timeBaseHandle,
                                                          TSync_TransmitGlobalTimeCallback cb) {
    return ptpCommonMock.TSync_RegisterTransmitGlobalTimeCallback(timeBaseHandle, cb);
}

void TSync_Close(void) {
    ptpCommonMock.TSync_Close();
}

void TSync_CloseTimebase(TSync_TimeBaseHandleType timeBaseHandle) {
    return ptpCommonMock.TSync_CloseTimebase(timeBaseHandle);
}

TSync_TimeBaseHandleType TSync_OpenTimebase(TSync_SynchronizedTimeBaseType timeBaseId) {
    return ptpCommonMock.TSync_OpenTimebase(timeBaseId);
}

TSync_ReturnType Score_PtpTransmitGlobalTimeCallback(TSync_SynchronizedTimeBaseType domainId) {
    return ptpCommonMock.Score_PtpTransmitGlobalTimeCallback(domainId);
}

TSync_ReturnType TSync_GetTimebaseConfiguration(TSync_TimeBaseHandleType timeBaseHandle, TSync_Role role_requested,
                                                TSync_TimeBaseConfiguration* config) {
    return ptpCommonMock.TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, config);
}

#endif /* SCORE_PTP_COMMON_MOCK_H */