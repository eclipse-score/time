/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "inc/score_ptp_subtlv.h"
#include "inc/score_ptp_arith.h"
#include "inc/score_ptp_common.h"

/** @brief Consolidates the all the provider timing information to compute
 * PreciseOriginTime
 */
Score_PtpProviderDataType providerData;

/** @brief Type for storing the configuration information and information that
 * is derived from the configuration. Configuration info mainly comes from the
 * command line.
 */
extern Score_PtpConfigType ptpConfig;

/** @brief Update the VirtualLocalTime for given message type.
 * @details
 * This functions gets the Virtual local Time from the TSync and updates it in
 * the provider data structure. Dependending on the argument - Sync or FUP
 * the VLT will be updated in the respective variable. These values are T0 and
 * T2 for the preciseOriginTime computation.
 * @param vltEgress     SCORE_PTP_EGRESS_T0 for updating T0
 *                      SCORE_PTP_EGRESS_T4 for updating T4.
 * @return None.
 */
void Score_PtpReadEgressVlt(Score_PtpEgressVltType vltEgress) {
    TSync_ReturnType returnValue = E_NOT_OK;
    TSync_VirtualLocalTimeType virtualLocalTime = {0u, 0u};

    /* Get the Current Virtual Local Time */
    returnValue = TSync_GetCurrentVirtualLocalTime(ptpConfig.timebaseHandle, &virtualLocalTime);

    if (E_NOT_OK == returnValue) {
        fprintf(
            stderr,
            "[ERROR] aptpd2: In function Score_PtpReadEgressVlt, failed to get Virtual Local Time from tsync-ptp-lib\n");

    } else {
        if (SCORE_PTP_EGRESS_T4 == vltEgress) {
            providerData.t4Vlt.nanosecondsHi = virtualLocalTime.nanosecondsHi;

            providerData.t4Vlt.nanosecondsLo = virtualLocalTime.nanosecondsLo;
        }
    }
} /* End of function Score_PtpReadEgressVlt */

/** @brief Update the GlobalTime and VirtualLocalTime for T0 and T0Vlt
 * respectively.
 * @details
 * This function gets the Virtual local Time and GlobalTime from TSync and
 * store it as T0 and T0Vlt. These values are used later to compute OriginTime.
 * @param None.
 * @return None.
 */
void Score_PtpBusGetGlobalTime(void) {
    TSync_ReturnType returnValue = E_NOT_OK;
    TSync_UserDataType userdata;
    TSync_MeasurementType measureData;
    TSync_VirtualLocalTimeType vlt;
    TSync_TimeStampType globalTime;

    memset((void *)(&userdata), 0u, sizeof(TSync_UserDataType));
    memset((void *)(&measureData), 0u, sizeof(TSync_MeasurementType));
    memset((void *)(&vlt), 0u, sizeof(TSync_VirtualLocalTimeType));
    memset((void *)(&globalTime), 0u, sizeof(TSync_TimeStampType));

    returnValue = TSync_BusGetGlobalTime(ptpConfig.timebaseHandle, &globalTime, &userdata, &measureData, &vlt);

    if (E_NOT_OK == returnValue) {
        fprintf(stderr, "[ERROR] aptpd2: Failed to get Global Time from TSync\n");
    } else {
#if PTPD_SCORE_DEBUG
        printf("\n [DEBUG] aptpd2: Global Time Read from timesync: ");
        printf("  %u", globalTime.secondsHi);
        printf(" : %u", globalTime.seconds);
        printf(" : %u", globalTime.nanoseconds);

        printf("\n [DEBUG] aptpd2: Virtual Local Time: ");
        printf("  %u", vlt.nanosecondsHi);
        printf(" : %u", vlt.nanosecondsLo);

        printf("\n [DEBUG] aptpd2: Userdata ");
        printf("  %u", userdata.userDataLength);
        printf("  %u", userdata.userByte0);
        printf("  %u", userdata.userByte1);
        printf(" : %u", userdata.userByte2);

#endif

        providerData.t0GlobalTime.secondsHi = globalTime.secondsHi;
        providerData.t0GlobalTime.seconds = globalTime.seconds;
        providerData.t0GlobalTime.nanoseconds = globalTime.nanoseconds;

        providerData.t0Vlt.nanosecondsHi = vlt.nanosecondsHi;
        providerData.t0Vlt.nanosecondsLo = vlt.nanosecondsLo;

        /* Fetch User data */
        providerData.followUpMsg.subTlv.userdata.userDataLength = userdata.userDataLength;
        providerData.followUpMsg.subTlv.userdata.userByte0 = userdata.userByte0;
        providerData.followUpMsg.subTlv.userdata.userByte1 = userdata.userByte1;
        providerData.followUpMsg.subTlv.userdata.userByte2 = userdata.userByte2;
    }
}

/** @brief Fetch T4Vlt and compute PreciseOriginTime .
 * @details
 * This functions fetches T4Vlt from tsync. Then based on T4Vlt and T0Vlt
 * (which is stored previously in Provider information), T0
 * is interpolated to compute the PreciseOriginTime.
 * @param None.
 * @return None.
 */
void Score_PtpProviderComputeOriginTime(void) {
    Score_PtpSignedTimeStampType globalTimeDiff;

    memset((void *)(&globalTimeDiff.ts), 0u, sizeof(TSync_TimeStampType));
    globalTimeDiff.sign = SCORE_PTP_POSITIVE;

    Score_PtpReadEgressVlt(SCORE_PTP_EGRESS_T4);

    globalTimeDiff.ts.secondsHi = providerData.t0GlobalTime.secondsHi;
    globalTimeDiff.ts.seconds = providerData.t0GlobalTime.seconds;
    globalTimeDiff.ts.nanoseconds = providerData.t0GlobalTime.nanoseconds;

    Score_PtpInterpolateTime(&providerData.t4Vlt, &providerData.t0Vlt, &globalTimeDiff, &providerData.followUpMsg.preciseOriginTimestamp);

#if PTPD_SCORE_DEBUG
    printf("Interpolated time to send");
    printf(" SecondsHI: %u",providerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi);
    printf(" Seconds: %u", providerData.followUpMsg.preciseOriginTimestamp.ts.seconds);
    printf(" Nanoseconds : %u\n", providerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds);

#endif

}

/** @brief Fetch PreciseOriginTime from Provider data-structure.
 * @details
 * This functions fetches origin time that was computed and stored in provider
 * data-structure based on T4Vlt and T0Vlt
 * @param originTimeOut      Pointer to store the computed origin time.
 * @return None.
 */
void Score_PtpWriteOriginTimeToBuffer(uint8_t *originTimeOut, uint8_t originTimeSize) {
    Score_PtpTimeStampType *originTimeStamp = NULL;

    if ((NULL != originTimeOut) && (originTimeSize == sizeof(Score_PtpTimeStampType))) {
        originTimeStamp = (Score_PtpTimeStampType *)(originTimeOut);

        originTimeStamp->secondsHi = providerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi;
        originTimeStamp->seconds = providerData.followUpMsg.preciseOriginTimestamp.ts.seconds;
        originTimeStamp->nanoseconds = providerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds;
    }
}

/** @brief Callback function to indicate Immediate time transmission.
 * @details
 * This function is called by TSync to indicate that immediate time transmission
 * is needed. It sets immediateTimeTransmission variable to TRUE. This will enable the time
 * transmission on the Bus in the next cycle.
 * @param[in]  domainId        The domain id for which time has to be
 * transmitted.
 * @return E_OK if successful, E_NOT_OK otherwise.
 */
TSync_ReturnType Score_PtpTransmitGlobalTimeCallback(TSync_SynchronizedTimeBaseType domainId) {
    TSync_ReturnType returnValue = E_NOT_OK;

    /* This condition is not really necessary, since the ptpd instance is always
    run for single domain */
    if (domainId == ptpConfig.timebaseId) {

        /* New time update received from TSync. Immediately send the time
        over Ethernet bus */
        providerData.immediateTimeTransmission = SCORE_PTP_TRUE;
        returnValue = E_OK;
    }

    return returnValue;
}

/** @brief Returns if Immediate time transmission is set to TRUE.
 * @details
 * This function returns if the Immediate time transmission is set to TRUE.
 * @param[in]  None
 * @return TRUE if Immediate time transmission is TRUE, FALSE otherwise.
 */
Score_PtpBooleanType Score_PtpIsImmediateTimeTriggerTrue(void) {
    return providerData.immediateTimeTransmission;
}

/** @brief Resets the Immediate time transmission flag to FALSE.
 * @details
 * This function resets the Immediate time transmission flag to FALSE. It shall
 * be called as soon as the transmission is initiated.
 * @param None.
 * @return None.
 */
void Score_PtpResetImmediateTimeTrigger(void) {
    providerData.immediateTimeTransmission = SCORE_PTP_FALSE;
}
