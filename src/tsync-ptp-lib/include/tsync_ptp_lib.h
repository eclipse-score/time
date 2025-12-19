/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef TSYNC_PTP_LIB_H_INCLUDED
#define TSYNC_PTP_LIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// coverity[autosar_cpp14_a1_1_1_violation] This library provides a C API, the use of C headers is is acceptable.

#include "tsync_ptp_lib_types.h"

/**
 * @brief Performs basic initialization of the tsync-ptp-lib
 *
 * @return TSync_ReturnType         E_OK:   successful
 *                              E_NOT_OK:   failed
 */
TSync_ReturnType TSync_Open(void);

/**
 * @brief Shuts down the tsync-ptp-lib
 */
void TSync_Close(void);

/**
 * @brief Opens a timebase for reading/writing
 *
 * @param timeBaseId The timebase identifier of the timebase to open.
 * @return TSync_TimeBaseHandleType Timebase handle or TSYNC_INVALID_HANDLE,
 * if the timebase could not be openend
 */
TSync_TimeBaseHandleType TSync_OpenTimebase(TSync_SynchronizedTimeBaseType timeBaseId);

/**
 * @brief Closes and invalidates a given timebase handle
 *
 * @param timeBaseHandle
 */
void TSync_CloseTimebase(TSync_TimeBaseHandleType timeBaseHandle);

/** @brief Allows the Time Base Provider Modules to forward a new Global Time
 * tuple.
 * @details
 * This function shall forward a new Global Time tuple.
 * @param[in] timeBaseHandle    Handle to Time Base
 * @param[in] globalTimePtr     New Global Time value
 * @param[in] userDataPtr       New User Data (if not NULL)
 * @param[in] measureDataPtr    New measurement data
 * @param[in] localTimePtr      Value of the Virtual Local Time associated to
 *                              the new Global Time
 * @return TSync_ReturnType         E_OK:   successful
 *                              E_NOT_OK:   failed
 */
TSync_ReturnType TSync_BusSetGlobalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                        const TSync_TimeStampType* globalTimePtr, const TSync_UserDataType* userDataPtr,
                                        const TSync_MeasurementType* measureDataPtr,
                                        const TSync_VirtualLocalTimeType* localTimePtr);

/** @brief Allows the Time Base Provider and Consumer Module to get a Global Time
 * tuple.
 * @details
 * This function shall get a new Global Time tuple.
 * @param[in] timeBaseHandle    Handle to Time Base
 * @param[in] globalTimePtr     New Global Time value
 * @param[in] userDataPtr       New User Data (if not NULL)
 * @param[in] measureDataPtr    New measurement data
 * @param[in] localTimePtr      Value of the Virtual Local Time associated to
 *                              the new Global Time
 * @return TSync_ReturnType         E_OK:   successful
 *                              E_NOT_OK:   failed
 */
TSync_ReturnType TSync_BusGetGlobalTime(TSync_TimeBaseHandleType timeBaseHandle, TSync_TimeStampType* globalTimePtr,
                                        TSync_UserDataType* userDataPtr, TSync_MeasurementType* measureDataPtr,
                                        TSync_VirtualLocalTimeType* localTimePtr);

/** @brief Returns the Virtual Local Time of the referenced Time Base
 *
 * @param[in]   timeBaseId    Time Base reference
 * @param[out]  localTimePtr  Current Virtual Local Time value
 * @return TSync_ReturnType         E_OK:   successful
 *                              E_NOT_OK:   failed
 */
TSync_ReturnType TSync_GetCurrentVirtualLocalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                                  TSync_VirtualLocalTimeType* localTimePtr);

/**
 * @brief Registers a function for triggering transmission of timestamp updates on the bus
 * @details
 * Registered callbacks will be automatically unregistered by calling TSync_CloseTimebase
 * Registering the same callback twice will fail.
 * @param[in] timeBaseHandle    Handle to Time Base
 * @param[in] cb                Callback function
 * @return TSync_ReturnType         E_OK:   successful
 *                              E_NOT_OK:   failed
 */
TSync_ReturnType TSync_RegisterTransmitGlobalTimeCallback(TSync_TimeBaseHandleType timeBaseHandle,
                                                          TSync_TransmitGlobalTimeCallback cb);

/**
 * @brief Sets the timeout status bit to 1 for the given timebase
 * @details
 * The TimeBase consumer independently monitors for the timeout periodically. When no timebase
 * update is received for <timeout seconds>, this function is called to
 * set the timeout bit.
 * @param[in] timeBaseHandle    Handle to Time Base
 * @return TSync_ReturnType         E_OK:   successful
 *                              E_NOT_OK:   failed
 */
TSync_ReturnType TSync_SetTimeoutStatus(TSync_TimeBaseHandleType timeBaseHandle);

/**
 * @brief Gets the timebase configuration for the given timebase
 * @details
 * The configuration for the given timebase is read and filled into the passed in configuration
 * structure.
 * @param[in]  timeBaseHandle                 Handle to Time Base
 * @param[in]  role_requested                 Role of the Time Base
 * @param[out] TSync_TimeBaseConfiguration    timebase configuration
 * @return TSync_ReturnType                       E_OK:   successful
 *                                            E_NOT_OK:   failed
 */
TSync_ReturnType TSync_GetTimebaseConfiguration(TSync_TimeBaseHandleType timeBaseHandle, TSync_Role role_requested,
                                                TSync_TimeBaseConfiguration* config);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TSYNC_PTP_LIB_H_INCLUDED */
