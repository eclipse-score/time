/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "tsync_ptp_lib.h"

#define TSYNC_TIMEBASE_SHMEM_SIZE 42
static uint8_t tsync_timebase_data[TSYNC_TIMEBASE_SHMEM_SIZE];

extern "C" {

TSync_VirtualLocalTimeType tsync_ptp_lib_mock_vlt_to_inject;
bool tsync_ptp_lib_mock_return_invalid_handle = false;
TSync_ReturnType tsync_transmit_global_time_return_value = E_NOT_OK;
TSync_ReturnType tsync_get_current_vlt_return_value = E_OK;

TSync_ReturnType TSync_Open(void) {
    return E_OK;
}

void TSync_Close(void) {
}

TSync_TimeBaseHandleType TSync_OpenTimebase(TSync_SynchronizedTimeBaseType timeBaseId) {
    (void)timeBaseId;
    if (tsync_ptp_lib_mock_return_invalid_handle) {
        return TSYNC_INVALID_HANDLE;
    } else {
        TSync_TimeBaseHandleType h = static_cast<TSync_TimeBaseHandleType>(&tsync_timebase_data);
        return h;
    }
}

void TSync_CloseTimebase(TSync_TimeBaseHandleType timeBaseHandle) {
    (void)timeBaseHandle;
}

TSync_ReturnType TSync_GetCurrentVirtualLocalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                                  TSync_VirtualLocalTimeType* localTimePtr) {
    (void)timeBaseHandle;

    localTimePtr->nanosecondsHi = tsync_ptp_lib_mock_vlt_to_inject.nanosecondsHi;
    localTimePtr->nanosecondsLo = tsync_ptp_lib_mock_vlt_to_inject.nanosecondsLo;

    return tsync_get_current_vlt_return_value;
}

TSync_ReturnType TSync_TransmitGlobalTime(TSync_TimeBaseHandleType timeBaseHandle) {
    (void)timeBaseHandle;
    return tsync_transmit_global_time_return_value;
}
}
