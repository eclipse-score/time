/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "inc/score_ptp_common.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "../../dep/constants_dep.h"
#include "../../ptp_datatypes.h"

/** @brief Type for storing the configuration information and information that
 * is derived from the configuration. Configuration info mainly comes from the
 * command line.
 */
Score_PtpConfigType ptpConfig;

/** @brief Defines the all the received information - both Sync and Fup message
 * fields that are required for the calculation of globaltime
 */
extern Score_PtpConsumerDataType consumerData;

/** @brief Consolidates the all the provider timing information to compute
 * PreciseOriginTime
 */
extern Score_PtpProviderDataType providerData;

/** @brief Updates the global variable for CRC time flags.
 * @details
 * This function updates the Bit masks in global variable according to
 * configured time flags.
 * @param[in]  None
 * @return None.
 */
static void Score_PtpUpdateCrcTimeFlags(void) {
    if (ptpConfig.timebaseConfig.crcFlags.message_length)
    {
        SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK);
    }
    if (ptpConfig.timebaseConfig.crcFlags.domain_number)
    {
        SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK);
    }
    if (ptpConfig.timebaseConfig.crcFlags.correction_field)
    {
        SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK);
    }
    if (ptpConfig.timebaseConfig.crcFlags.source_port_identity)
    {
        SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK);
    }
    if (ptpConfig.timebaseConfig.crcFlags.sequence_id)
    {
        SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK);
    }
    if (ptpConfig.timebaseConfig.crcFlags.precise_origin_timestamp)
    {
        SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK);
    }
}

/** @brief Init function to update configuration information.
 * @details
 * This function stores the configuration information into a global structure.
 * The information is received from the command line options.
 * @param[in]  pdelay        Static Propagation delay.
 * @param[in]  domainNumber  Time Base Identifier.
 * @param[in]  isProvider    If the ptpd instance is Provider.
 * @return Score_PtpReturnType  SCORE_PTP_E_OK for successful initialization
 *                             SCORE_PTP_E_NOT_OK otherwise.
 */
Score_PtpReturnType Score_PtpInitialize(double pdelay, uint16_t domainNumber, Score_PtpBooleanType isProvider) {
    Score_PtpReturnType returnValue = SCORE_PTP_E_NOT_OK;

    /* For the received TimeBase, the handle must be derived from the
    TSync-Lib. This handle will be used for making tsync calls related to
    the above timebaseId */
    if (E_OK == TSync_Open()) {
        ptpConfig.timebaseHandle = TSync_OpenTimebase((TSync_SynchronizedTimeBaseType)(domainNumber));

        if (NULL != ptpConfig.timebaseHandle) {
            /* Update the timebase id */
            ptpConfig.timebaseId = domainNumber;

            if (isProvider) {
                if (E_OK != TSync_RegisterTransmitGlobalTimeCallback(ptpConfig.timebaseHandle,
                                                                            Score_PtpTransmitGlobalTimeCallback)) {
                    return returnValue;
                }
            }

        } else {
            ptpConfig.timebaseHandle = NULL;
            fprintf(stderr,
                    "[ERROR] aptpd2: Timebase handle is NULL. Basic "
                    "initialization failed.\n");
            return returnValue;
        }
    } else {
        ptpConfig.timebaseHandle = NULL;
        fprintf(stderr, "[ERROR] aptpd2: TSync_Open failed. Basic initialization failed.\n");
        return returnValue;
    }

    if (SCORE_PTP_E_OK == Score_PtpConfigure(isProvider)) {
        /* Update pdelay - Convert the received pdelay into TimeStamp type */
        ptpConfig.pdelay.ts.secondsHi = 0u;
        ptpConfig.pdelay.ts.seconds = trunc(pdelay);
        ptpConfig.pdelay.ts.nanoseconds = (pdelay - (ptpConfig.pdelay.ts.seconds + 0.0)) * 1E9;

        /* Updating the default sign as POSITIVE */
        ptpConfig.pdelay.sign = SCORE_PTP_POSITIVE;

        /* Update the default Sync-State to INVALID */
        consumerData.syncState = SCORE_PTP_STATE_INVALID;

        returnValue = SCORE_PTP_E_OK;

#ifdef PTPD_SCORE_DEBUG
        printf("\n[DEBUG] aptpd2: The config values are : ");
        printf(
            "\n[DEBUG] aptpd2: Pdelay : %d secondsHi, %d seconds and %d "
            "nanoseconds",
            ptpConfig.pdelay.ts.secondsHi, ptpConfig.pdelay.ts.seconds, ptpConfig.pdelay.ts.nanoseconds);

        printf("\n[DEBUG] aptpd2: DomainNumber : %d", ptpConfig.timebaseId);
        printf("\n[DEBUG] aptpd2: Crc support(For Provider) : %d", ptpConfig.timebaseConfig.crcSupport);
        printf("\n[DEBUG] aptpd2: Crc Validation(For Consumer) : %d", ptpConfig.timebaseConfig.crcValidation);
        printf("\n[DEBUG] aptpd2: messageCompliance : %d", ptpConfig.timebaseConfig.messageCompliance);
        printf("\n[DEBUG] aptpd2: followUpTimeSubTLV : %d",
               ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV);
        printf("\n[DEBUG] aptpd2: followUpStatusSubTLV : %d",
               ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV);
        printf("\n[DEBUG] aptpd2: followUpUserDataSubTLV : %d",
               ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV);
        printf("\n[DEBUG] aptpd2: correction_field : %d",
               ptpConfig.timebaseConfig.crcFlags.correction_field);
        printf("\n[DEBUG] aptpd2: domain_number : %d",
               ptpConfig.timebaseConfig.crcFlags.domain_number);
        printf("\n[DEBUG] aptpd2: message_length : %d",
               ptpConfig.timebaseConfig.crcFlags.message_length);
        printf("\n[DEBUG] aptpd2: precise_origin_timestamp : %d",
               ptpConfig.timebaseConfig.crcFlags.precise_origin_timestamp);
        printf("\n[DEBUG] aptpd2: sequence_id : %d",
               ptpConfig.timebaseConfig.crcFlags.sequence_id);
        printf("\n[DEBUG] aptpd2: source_port_identity : %d",
               ptpConfig.timebaseConfig.crcFlags.source_port_identity);
        printf("\n[DEBUG] aptpd2: syncPeriodMs : %d",
               ptpConfig.timebaseConfig.syncPeriodMs);
#endif
    }

    return returnValue;
}

/** @brief Deinit function to perform graceful shutdown.
 * @details
 * This functions performs all the activities associated with graceful shutdown
 * - Like close the connection with TSync, set the tsync handle to NULL.
 * @param None.
 * @return None.
 */
void Score_PtpDeinitialize(void) {
    /* Close the TimeBase */
    TSync_CloseTimebase(ptpConfig.timebaseHandle);

    /* Set the timebase handle to NULL */
    ptpConfig.timebaseHandle = NULL;

    /* Update the Sync-State to Invalid */
    consumerData.syncState = SCORE_PTP_STATE_INVALID;

    /* Close the connection to TSync */
    TSync_Close();

}

/** @brief Configure function to read configuration from tsync-ptp-lib.
 * @details
 * This function retrieves the configuration data from tsync-ptp-lib during
 * the startup. The configuation is stored into the global configuration
 * structure.
 * @return Score_PtpReturnType  SCORE_PTP_E_OK for successful config update
 *                             SCORE_PTP_E_NOT_OK otherwise.
 */
Score_PtpReturnType Score_PtpConfigure(Score_PtpBooleanType isProvider) {
    TSync_Role roleRequested = isProvider ? TSYNC_ROLE_MASTER : TSYNC_ROLE_SLAVE;

    if (E_OK == TSync_GetTimebaseConfiguration(ptpConfig.timebaseHandle, roleRequested, &ptpConfig.timebaseConfig)) {
        /* If AUTOSAR packet is chosen write/read SubTLVs too to/from ptpd buffer */
        if (TSYNC_MC_IEEE8021AS_AUTOSAR == ptpConfig.timebaseConfig.messageCompliance) {
            ptpConfig.configGlobals.followupLength = SCORE_PTP_FUP_PACKET_LENGTH_FIXED_AUTOSAR;

            /** Check all the SubTLVs configured and accordingly update the
             * follow-up length */
            /* Do not consider TimeSecured SubTLV if overall crcValidation is disabled */
            if(isProvider) {
                /* For the Provider, TimeSecured SubTLV is enabled if crcSupport is enabled */
                if((TSYNC_CRC_SUPPORTED == ptpConfig.timebaseConfig.crcSupport) &&
                   (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV)) {
                    ptpConfig.configGlobals.followupLength += sizeof(Score_PtpSubtlvTimeSecuredType);
                    ptpConfig.configGlobals.configSubTlvLength += sizeof(Score_PtpSubtlvTimeSecuredType);
                    Score_PtpUpdateCrcTimeFlags();
                }
            }
            else {
                 /* For the Consumer, TimeSecured SubTLV is enabled if crcValidation is other than TSYNC_CRC_NOT_VALIDATED */
                if((TSYNC_CRC_NOT_VALIDATED != ptpConfig.timebaseConfig.crcValidation) &&
                   (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV)) {
                    ptpConfig.configGlobals.followupLength += sizeof(Score_PtpSubtlvTimeSecuredType);
                    ptpConfig.configGlobals.configSubTlvLength += sizeof(Score_PtpSubtlvTimeSecuredType);
                    Score_PtpUpdateCrcTimeFlags();
                }
            }
            if (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV) {
                ptpConfig.configGlobals.followupLength += sizeof(Score_PtpSubtlvStatusType);
                ptpConfig.configGlobals.configSubTlvLength += sizeof(Score_PtpSubtlvStatusType);
            }
            if (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV) {
                ptpConfig.configGlobals.followupLength += sizeof(Score_PtpSubtlvUserdataType);
                ptpConfig.configGlobals.configSubTlvLength += sizeof(Score_PtpSubtlvUserdataType);
            }
        }
        /* Use default follow-up length as configured as per IEEE8021AS */
        else {
            ptpConfig.configGlobals.followupLength = SCORE_PTP_FUP_PACKET_LENGTH_IEEE8021AS;
        }

        /* Check if the syncPeriodMs is configured or not */
        if(0 == ptpConfig.timebaseConfig.syncPeriodMs) {
            /* Set the default value to 1 second */
            ptpConfig.timebaseConfig.syncPeriodMs = SCORE_PTP_DEFAULT_SYNC_PERIOD;
        }
        return SCORE_PTP_E_OK;
    }
    fprintf(stderr, "[ERROR] aptpd2: Could not fetch configuration via TSync_GetTimebaseConfiguration\n");
    return SCORE_PTP_E_NOT_OK;
}

/** @brief Stores the followup header information.
 * @details
 * This function reads the followup header information from the ptpd data structures(open source data structure)
 * into the global structure used by SCORE extensions.The values in this header files are as received by the protocol
 * expect that they are converted from big endian to host format.
 * @return None.
 */
void Score_PtpExtractFupHeaderInfo(const struct MsgHeaderTag* followupHeaderInfoIn, Score_PtpBooleanType isProvider) {
    if (NULL != followupHeaderInfoIn) {
        if (isProvider) {
            providerData.followUpMsg.followupHeaderInfo.transportSpecific = followupHeaderInfoIn->transportSpecific;
            providerData.followUpMsg.followupHeaderInfo.messageType = followupHeaderInfoIn->messageType;
            providerData.followUpMsg.followupHeaderInfo.reserved0 = followupHeaderInfoIn->reserved0;
            providerData.followUpMsg.followupHeaderInfo.versionPtp = followupHeaderInfoIn->versionPTP;
            providerData.followUpMsg.followupHeaderInfo.messageLength = followupHeaderInfoIn->messageLength;
            providerData.followUpMsg.followupHeaderInfo.domainNumber = followupHeaderInfoIn->domainNumber;
            providerData.followUpMsg.followupHeaderInfo.flagField0 = followupHeaderInfoIn->flagField0;
            providerData.followUpMsg.followupHeaderInfo.flagField1 = followupHeaderInfoIn->flagField1;
            providerData.followUpMsg.followupHeaderInfo.correctionField =
                ((followupHeaderInfoIn->correctionField.msb << 32) | (followupHeaderInfoIn->correctionField.lsb));
            memcpy((&providerData.followUpMsg.followupHeaderInfo.sourcePortIdentity),
                   (&followupHeaderInfoIn->sourcePortIdentity), sizeof(PortIdentity));
            providerData.followUpMsg.followupHeaderInfo.sequenceId = followupHeaderInfoIn->sequenceId;
            providerData.followUpMsg.followupHeaderInfo.controlField = followupHeaderInfoIn->controlField;
            providerData.followUpMsg.followupHeaderInfo.logMessageInterval = followupHeaderInfoIn->logMessageInterval;
        } else {
            consumerData.followUpMsg.followupHeaderInfo.transportSpecific = followupHeaderInfoIn->transportSpecific;
            consumerData.followUpMsg.followupHeaderInfo.messageType = followupHeaderInfoIn->messageType;
            consumerData.followUpMsg.followupHeaderInfo.reserved0 = followupHeaderInfoIn->reserved0;
            consumerData.followUpMsg.followupHeaderInfo.versionPtp = followupHeaderInfoIn->versionPTP;
            consumerData.followUpMsg.followupHeaderInfo.messageLength = followupHeaderInfoIn->messageLength;
            consumerData.followUpMsg.followupHeaderInfo.domainNumber = followupHeaderInfoIn->domainNumber;
            consumerData.followUpMsg.followupHeaderInfo.flagField0 = followupHeaderInfoIn->flagField0;
            consumerData.followUpMsg.followupHeaderInfo.flagField1 = followupHeaderInfoIn->flagField1;
            consumerData.followUpMsg.followupHeaderInfo.correctionField =
                ((followupHeaderInfoIn->correctionField.msb << 32) | (followupHeaderInfoIn->correctionField.lsb));
            memcpy((void*)(&consumerData.followUpMsg.followupHeaderInfo.sourcePortIdentity),
                   (void*)(&followupHeaderInfoIn->sourcePortIdentity), sizeof(PortIdentity));
            consumerData.followUpMsg.followupHeaderInfo.sequenceId = followupHeaderInfoIn->sequenceId;
            consumerData.followUpMsg.followupHeaderInfo.controlField = followupHeaderInfoIn->controlField;
            consumerData.followUpMsg.followupHeaderInfo.logMessageInterval = followupHeaderInfoIn->logMessageInterval;
        }
    }
}
