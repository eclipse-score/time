/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "inc/score_ptp_consumer.h"

#include <stdio.h>
#include <string.h>

#include "inc/score_ptp_types.h"
#include "inc/score_ptp_arith.h"
#include "inc/score_ptp_common.h"
#include "inc/score_ptp_subtlv.h"
#include "inc/score_ptp_config.h"

/** @brief  The structure that is used to store the information for computation
 * of globalTime */
Score_PtpConsumerDataType consumerData;

/** @brief Type for storing the configuration information and information that
 * is derived from the configuration. Configuration info mainly comes from the
 * command line.
 */
extern Score_PtpConfigType ptpConfig;

/** @brief Function to update the follow-up information received .
 * @details
 * This function stores the follow-up information into a global data structure.
 * The information is originally received from the follow-up message.
 * @param[in]  originTimeIn          Precise Origin Time.
 * @param[in]  corrTimeIn            Correction field.
 * @param[in]  fupInfoIn             Pointer to buffer containing fup info.
 * @return None.
 */
void Score_PtpReadFupInfoFromBuffer(const uint8_t *originTimeIn, const uint8_t *corrTimeIn, const uint8_t *fupInfoIn) {
    if ((NULL != fupInfoIn) && (NULL != originTimeIn)) {
        /* Type cast the information received to TimeStamp type */
        TSync_TimeStampType *correctionField = (TSync_TimeStampType *)corrTimeIn;

        Score_PtpTimeStampType *originTime = (Score_PtpTimeStampType *)originTimeIn;

        /* Proceed only if a valid Sync message VLT was updated earlier */
        if (SCORE_PTP_STATE_SYNC_RCVD == consumerData.syncState) {
            /* Update the Correction Field Value in Consumer structure */
            consumerData.correctionField.ts.secondsHi = correctionField->secondsHi;
            consumerData.correctionField.ts.seconds = correctionField->seconds;
            consumerData.correctionField.ts.nanoseconds = correctionField->nanoseconds;

            /* Set default sign of the time to POSITIVE */
            consumerData.correctionField.sign = SCORE_PTP_POSITIVE;

            /* Update the Origin Time in Consumer structure */
            consumerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi = originTime->secondsHi;
            consumerData.followUpMsg.preciseOriginTimestamp.ts.seconds = originTime->seconds;
            consumerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds = originTime->nanoseconds;

            /* Set default sign of the time to POSITIVE */
            consumerData.followUpMsg.preciseOriginTimestamp.sign = SCORE_PTP_POSITIVE;

            /* Forward the time values to timesync only if SubTLVs were read successfully */
            if (Score_PtpReadSubTlvFromBuffer(fupInfoIn)) {
                Score_PtpBusSetGlobalTime();
            } else {
                fprintf(
                    stderr,
                    "[ERROR] aptpd2: Autosar SubTLV Validation failure. Follow-Up message will be dropped. \n");
            }
        } else {
            fprintf(stderr,
                    "[ERROR] aptpd2: The Sync-State is not in SYNC-RECEIVED. FUP message will not be processed \n");
        }
    }
} /* End of function Score_PtpReadFupInfoFromBuffer */

/** @brief Update the VirtualLocalTime for given message type.
 * @details
 * This functions gets the Virtual local Time from the TSync and updates it in
 * the consumer data structure. Dependending on the argument - Sync or FUP
 * the VLT will be updated in the respective variable. These values are T1 and
 * T2 for the globalTime computation.
 * @param vltIngress    SCORE_PTP_INGRESS_T1 for updating T1
 *                      SCORE_PTP_INGRESS_T2 for updating T2.
 * @return None.
 */
void Score_PtpReadIngressVlt(Score_PtpIngressVltType vltIngress) {
    TSync_ReturnType returnValue = E_NOT_OK;
    TSync_VirtualLocalTimeType virtualLocalTime = {0u, 0u};

    /* Get the Current Virtual Local Time */
    returnValue = TSync_GetCurrentVirtualLocalTime(ptpConfig.timebaseHandle, &virtualLocalTime);

    if (E_NOT_OK == returnValue) {
        fprintf(
            stderr,
            "[ERROR] aptpd2: In function Score_PtpReadIngressVlt, failed to get Virtual Local Time from tsync-ptp-lib\n");

        /* Update the Sync-State to Invalid */
        consumerData.syncState = SCORE_PTP_STATE_INVALID;
    } else {
        /* Get TSync Time Stamp and update in the appropriate field */
        switch (vltIngress) {
            case SCORE_PTP_INGRESS_T1:
                consumerData.t1Vlt.nanosecondsHi = virtualLocalTime.nanosecondsHi;

                consumerData.t1Vlt.nanosecondsLo = virtualLocalTime.nanosecondsLo;

                /* Update the Sync-State to Sync-Received */
                consumerData.syncState = SCORE_PTP_STATE_SYNC_RCVD;
                break;

            case SCORE_PTP_INGRESS_T2:
                consumerData.t2Vlt.nanosecondsHi = virtualLocalTime.nanosecondsHi;

                consumerData.t2Vlt.nanosecondsLo = virtualLocalTime.nanosecondsLo;

                /* Update the Sync-State to Fup received */
                consumerData.syncState = SCORE_PTP_STATE_FUP_RCVD;
                break;
            default:
                break;
        }
    }
} /* End of function Score_PtpReadIngressVlt */

/** @brief Computes the globalTime and calls BusSetGlobalTime.
 * @details
 * This functions performs the globalTime computation and makes call to
 * BusSetGlobalTime. The fomula for globalTime computation is:
 *    globalTime = PreciseOriginTime + correctionfield + (T2vlt-T1vlt) + Pdelay;
 *    localTime = T2vlt,
 *    measureData = Pdelay
 * @param None
 * @return None.
 */
void Score_PtpBusSetGlobalTime(void) {
    TSync_ReturnType returnValue = E_NOT_OK;

    Score_PtpSignedTimeStampType timeStampDiff = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType offset = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType syncTime = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType addedTime = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType finalGlobalTime = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType syncTimeStamp = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType fupTimeStamp = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    uint64_t syncRxTimeStamp = 0u;
    uint64_t fupRxTimeStamp = 0u;

    /* Variables that are used to send time to TSync Lib */
    TSync_TimeStampType globalTime = {0u, 0u, 0u, 0u};
    TSync_UserDataType userdata = {0u, 0u, 0u, 0u};
    TSync_MeasurementType measureData = {0u};
    TSync_VirtualLocalTimeType localTime = {0u, 0u};

    /* Update the T2 Time - Virtual Local Time Type */
    Score_PtpReadIngressVlt(SCORE_PTP_INGRESS_T2);

    /* Proceed only if a valid FUP message VLT was successful */
    /* Protection to variable consumerData is not needed as the
    control-flow is synchronous */
    if (SCORE_PTP_STATE_FUP_RCVD == consumerData.syncState) {
        /* Update the pdelay */
        measureData.pathDelay = ptpConfig.pdelay.ts.nanoseconds;

        /* Update the virtual local time to send to TSync-Lib */
        localTime.nanosecondsLo = consumerData.t2Vlt.nanosecondsLo;
        localTime.nanosecondsHi = consumerData.t2Vlt.nanosecondsHi;

        /* Left shift nanoseconds high position by 32 times and perform OR operation with nanoseconds low value -- for
         * T1 */
        syncRxTimeStamp = (uint64_t)((((uint64_t)consumerData.t1Vlt.nanosecondsHi) << 32u) |
                                     consumerData.t1Vlt.nanosecondsLo);

        /* Left shift nanoseconds high position by 32 times and perform OR operation with nanoseconds low value -- for
         * T2 */
        fupRxTimeStamp = (uint64_t)((((uint64_t)consumerData.t2Vlt.nanosecondsHi) << 32u) |
                                    consumerData.t2Vlt.nanosecondsLo);

        /* Convert and store the current time in Eth_TimeIntDiffType structure */
        Score_PtpConvertToTimeStamp(syncRxTimeStamp, 0u, &timeStampDiff);

        syncTimeStamp.ts = timeStampDiff.ts;

        /* Convert and store the current time in Eth_TimeIntDiffType structure */
        Score_PtpConvertToTimeStamp(fupRxTimeStamp, 0u, &timeStampDiff);

        fupTimeStamp.ts = timeStampDiff.ts;

        /* Obtain time difference between Provider and Consumer by subtracting Ingress Time from PreciseOriginTimeStamp
         * obtained in Followup frame */
        Score_PtpCalculateTimeDiff(&fupTimeStamp, &syncTimeStamp, &offset);

        /* Add the offset time to Consumer to sync time with Providers's */
        Score_PtpCalculateTimeSum(&consumerData.followUpMsg.preciseOriginTimestamp, &offset, &syncTime);

        /* Add Path delay to the time obtained*/
        Score_PtpCalculateTimeSum(&syncTime, &ptpConfig.pdelay, &addedTime);

        /* Add CorrectionField to the time difference */
        Score_PtpCalculateTimeSum(&addedTime, &consumerData.correctionField, &finalGlobalTime);

        /* Assign the obtained time to a variable of type TSync_TimeStampType */
        globalTime.nanoseconds = finalGlobalTime.ts.nanoseconds;
        globalTime.seconds = finalGlobalTime.ts.seconds;
        globalTime.secondsHi = finalGlobalTime.ts.secondsHi;

        userdata.userDataLength = consumerData.followUpMsg.subTlv.userdata.userDataLength;
        userdata.userByte0 = consumerData.followUpMsg.subTlv.userdata.userByte0;
        userdata.userByte1 = consumerData.followUpMsg.subTlv.userdata.userByte1;
        userdata.userByte2 = consumerData.followUpMsg.subTlv.userdata.userByte2;

#if PTPD_SCORE_DEBUG
        printf("\n[DEBUG] aptpd2: Sending the Global Time to TSync %u secondsHI, %u seconds and %u nanoseconds",
               globalTime.secondsHi, globalTime.seconds, globalTime.nanoseconds);

        printf("\n[DEBUG] aptpd2: Virtual Local Time to TSync %u nanosecondsLo, %u nanosecondsHi",
               localTime.nanosecondsLo, localTime.nanosecondsHi);

        printf(
            "\n[DEBUG] aptpd2: User Data Length = %u, userdata.userByte0 = %u, userdata.userByte1 = %u, userdata.userByte2 = %u",
            userdata.userDataLength, userdata.userByte0, userdata.userByte1, userdata.userByte2);
#endif

        returnValue =
            TSync_BusSetGlobalTime(ptpConfig.timebaseHandle, &globalTime, &userdata, &measureData, &localTime);

        if (E_NOT_OK == returnValue) {
            fprintf(stderr, "[ERROR] aptpd2: Call to TSync_BusSetGlobalTime failed\n");
        }
    } /* End of conditon Sync-State */
    else {
        fprintf(stderr, "[ERROR] aptpd2: The Sync-State is not in FUP-RECEIVED. No BusSetGlobalTime called \n");
    }
} /* End of function Score_PtpBusSetGlobalTime */
