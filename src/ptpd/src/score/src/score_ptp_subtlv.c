/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "../inc/score_ptp_subtlv.h"
#include "../inc/Std_Types.h"

#include <arpa/inet.h>
#include <string.h>
#include "inc/score_ptp_types.h"
#include "inc/score_ptp_config.h"

/* Stub implementation of CRC function - CRC functionality is currently not available (for more details see #94128) */
uint8 Crc_CalculateCRC8H2F(const uint8* Crc_DataPtr, uint32 Crc_Length, uint8 Crc_StartValue8, boolean Crc_IsFirstCall) {
    (void)Crc_DataPtr;
    (void)Crc_Length;
    (void)Crc_IsFirstCall;
    return Crc_StartValue8;
}

/** @brief Consolidates the all the provider timing information to compute
 * PreciseOriginTime
 */
extern Score_PtpProviderDataType providerData;

/** @brief  The structure that is used to store the information for computation
 * of globalTime */
extern Score_PtpConsumerDataType consumerData;

/** @brief Type for storing the configuration information and information that
 * is derived from the configuration. Configuration info mainly comes from the
 * command line.
 */
extern Score_PtpConfigType ptpConfig;

/** @brief Function to generate the DataId for the given sequenceId
 * @details
 * This function generates the DataId for the given sequenceId.
 * First the mod 16 operation is carried out on the sequenceId. Then the remainder
 * is used as index to fetch the FupDataId from the configured list of values.
 * @param[in]  number Sequence Id for the followup message(Provider or Consumer)
 * @return  Calculated DataId
 */
static inline uint8_t Score_PtpGetDataId(uint16_t sequenceId) {
    /* [PRS_TS_00112] The DataID shall be calculated as: DataID = DataIDList
    [Follow_Up.sequenceId mod 16], where DataIDList is given by configuration
    for the Follow_Up.(RS_TS_20061) */
    return ptpConfig.timebaseConfig.fupDataIdList[sequenceId % 16];
}

/** @brief Function to flip the bytes for 16 bit unsigned values.
 * @details
 * This function flips the 16bit value from host to network byte order.
 * @param[in]  number  16 bit unsigned number in host byte order
 * @return  16 bit unsigned number in network byte order.
 */
static inline uint16_t Score_PtpFlip16(uint16_t number) {
    return htons(number);
}

/** @brief Updates the SubTlvTimeSecured information into the ptpd buffer.
 * @details
 * This function is used by Provider to write the TimeSecured information to
 * outgoing buffer.
 * @param[out]  timeSecuredOut    Pointer to timeSecured array.
 * @return  Returns number of bytes written.
 */
static uint16_t Score_PtpWriteSubTLVTimeSecured(uint8_t *timeSecuredOut) {
    unsigned int index = 0u;
    uint8_t dataID = 0u;
    Score_PtpSubtlvTimeSecuredType timeSecured;
    uint8_t crcInputStream0[SCORE_PTP_TIMESECURED_CRC_TIME_0_STREAM_LENGTH];
    uint8_t crcInputStream1[SCORE_PTP_TIMESECURED_CRC_TIME_1_STREAM_LENGTH];

    memset(crcInputStream0, 0u, SCORE_PTP_TIMESECURED_CRC_TIME_0_STREAM_LENGTH);
    memset(crcInputStream1, 0u, SCORE_PTP_TIMESECURED_CRC_TIME_1_STREAM_LENGTH);

    timeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    timeSecured.subtlvLength = SCORE_PTP_FUP_SUBTLV_LEN_FIELD_TIMESECURED;

    timeSecured.crcTimeFlags = ptpConfig.configGlobals.crcTimeFlags;

    /* Update timeflags in crc stream */
    crcInputStream0[index++] = ptpConfig.configGlobals.crcTimeFlags;

    /* Update the remaining configuration elements in crc stream */
    if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK)) {
        /*[PRS_TS_00184] If applying the CRC calculation on multibyte message data, the
        byte order shall be in ascending order of the octets, i.e., the octet with the lowest offset
        shall be used first.*/
        crcInputStream0[index++] = (ptpConfig.timebaseId & SCORE_PTP_LOWER_BYTE_MASK);
        crcInputStream0[index++] = ((ptpConfig.timebaseId & SCORE_PTP_HIGHER_BYTE_MASK) >> 8);
    }
    if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK)) {
        memcpy(&crcInputStream0[index], &providerData.followUpMsg.followupHeaderInfo.sourcePortIdentity,
               SCORE_PTP_FUP_MESSAGE_HEADER_PORT_IDENTITY_LENGTH);
        index += SCORE_PTP_FUP_MESSAGE_HEADER_PORT_IDENTITY_LENGTH;
    }
    if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK)) {
        memcpy(&crcInputStream0[index], &providerData.followUpMsg.preciseOriginTimestamp,
               SCORE_PTP_FUP_MESSAGE_PRECISE_ORIGIN_TIME_LENGTH);
        index += SCORE_PTP_FUP_MESSAGE_PRECISE_ORIGIN_TIME_LENGTH;
    }
    /* Finally, update DataID in crc stream */
    dataID = Score_PtpGetDataId(providerData.followUpMsg.followupHeaderInfo.sequenceId);
    crcInputStream0[index++] = dataID;

    /* Is the stream length always 5 or it depends on the configuration */
    timeSecured.crcTime0 = Crc_CalculateCRC8H2F((uint8_t *)(&crcInputStream0[0]), index, 0u, SCORE_PTP_TRUE);

    /* Reset the index for next stream */
    index = 0u;

    /* Update timeflags in crc stream */
    crcInputStream1[index++] = ptpConfig.configGlobals.crcTimeFlags;

    if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK)) {
        crcInputStream1[index++] = (providerData.followUpMsg.followupHeaderInfo.messageLength & SCORE_PTP_LOWER_BYTE_MASK);
        crcInputStream1[index++] =
            ((providerData.followUpMsg.followupHeaderInfo.messageLength & SCORE_PTP_HIGHER_BYTE_MASK) >> 8);
    }
    if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK)) {
        memcpy(&crcInputStream1[index], &providerData.followUpMsg.followupHeaderInfo.correctionField,
               SCORE_PTP_FUP_MESSAGE_HEADER_CORRECTION_FIELD_LENGTH);
        index += SCORE_PTP_FUP_MESSAGE_HEADER_CORRECTION_FIELD_LENGTH;
    }
    if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK)) {
        crcInputStream1[index++] = (providerData.followUpMsg.followupHeaderInfo.sequenceId & SCORE_PTP_LOWER_BYTE_MASK);
        crcInputStream1[index++] =
            ((providerData.followUpMsg.followupHeaderInfo.sequenceId & SCORE_PTP_HIGHER_BYTE_MASK) >> 8);
    }

    /* Finally, update DataID in crc stream */
    crcInputStream1[index++] = dataID;

    timeSecured.crcTime1 =
        Crc_CalculateCRC8H2F((uint8_t *)(&crcInputStream1[0]), (uint8_t)(index), 0u, SCORE_PTP_TRUE);

    memcpy((void *)(timeSecuredOut), (void *)(&timeSecured), sizeof(Score_PtpSubtlvTimeSecuredType));
    return ((uint16_t)(sizeof(Score_PtpSubtlvTimeSecuredType)));
}

/** @brief Updates the SubTlv Userdata information into the ptpd buffer.
 * @details
 * This function is used by Provider to write the Userdata information to
 * outgoing buffer.
 * @param[out]  userdataOut    Pointer to userdata array.
 * @return Returns number of bytes written.
 */
static uint16_t Score_PtpWriteSubTLVUserdata(uint8_t *userdataOut) {
    Score_PtpSubtlvUserdataType userdata;
    uint8_t crcInputStream[SCORE_PTP_USERDATA_CRC_STREAM_LENGTH];
    memset(crcInputStream, 0u, SCORE_PTP_USERDATA_CRC_STREAM_LENGTH);

    /* Length of the Subtlv type - User data length */
    userdata.userdataLen = providerData.followUpMsg.subTlv.userdata.userDataLength;

    /* User data byte 0 */
    userdata.userdataByte0 = providerData.followUpMsg.subTlv.userdata.userByte0;

    /* User data byte 1 */
    userdata.userdataByte1 = providerData.followUpMsg.subTlv.userdata.userByte1;

    /* User data byte 2 */
    userdata.userdataByte2 = providerData.followUpMsg.subTlv.userdata.userByte2;

    if (TSYNC_CRC_SUPPORTED == ptpConfig.timebaseConfig.crcSupport) {
        /* SubTLV type */
        userdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;

        crcInputStream[0] = userdata.userdataLen;
        crcInputStream[1] = userdata.userdataByte0;
        crcInputStream[2] = userdata.userdataByte1;
        crcInputStream[3] = userdata.userdataByte2;

        if (ptpConfig.timebaseConfig.numFupDataIdEntries > 0u) {
            crcInputStream[4] = Score_PtpGetDataId(providerData.followUpMsg.followupHeaderInfo.sequenceId);
        }
        userdata.crcUserdata = Crc_CalculateCRC8H2F((uint8_t *)(&crcInputStream[0]), SCORE_PTP_USERDATA_CRC_STREAM_LENGTH,
                                                    0u, SCORE_PTP_TRUE);
    } else {
        /* SubTLV type */
        userdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC;

        /* Reserved field - CRC not supported */
        userdata.crcUserdata = 0u;
    }

    /* SubTLV length */
    userdata.subtlvLength = SCORE_PTP_FUP_SUBTLV_LEN_FIELD_USERDATA;

    memcpy((void *)(userdataOut), (void *)(&userdata), sizeof(Score_PtpSubtlvUserdataType));
    return ((uint16_t)(sizeof(Score_PtpSubtlvUserdataType)));
}

/** @brief Updates the Autosar TLV information into the ptpd buffer.
 * @details
 * This function is used by Provider to write the Autosar TLV information to
 * outgoing buffer.
 * @param[out]  autosarTlvBufferOut    Pointer to buffer where Autosar TLV can
 * be updated.
 * @return None.
 */
void Score_PtpWriteAutosarTlvToBuffer(uint8_t *autosarTlvBufferOut) {
    Score_PtpAutosarTlvType autosarTlv;
    Score_PtpAutosarTlvType *autosarTlvPtr = &autosarTlv;
    uint16_t tlvLength = 0u;
    uint8_t *subTLVPtr = NULL;

    /* Include SubTLV in outgoing buffer only if message compliance is TSYNC_MC_IEEE8021AS_AUTOSAR */
    if (TSYNC_MC_IEEE8021AS_AUTOSAR == ptpConfig.timebaseConfig.messageCompliance) {
        if (NULL != autosarTlvBufferOut) {
            /* Get the place */
            subTLVPtr = autosarTlvBufferOut + SCORE_PTP_FUP_AUTOSAR_TLV_INFO_LENGTH;

            /* Update information for TimeSecured SubTLV only if crcSupport is enabled */
            if ((TSYNC_CRC_SUPPORTED == ptpConfig.timebaseConfig.crcSupport) &&
                (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV)) {
                tlvLength = Score_PtpWriteSubTLVTimeSecured(subTLVPtr);
                subTLVPtr += SCORE_PTP_FUP_SUBTLV_TIMESECURED_LENGTH;
            }

            /* Update information for Status SubTLV */
            if (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV) {
                /* Write Status subtlv to the buffer - Todo */
                subTLVPtr += SCORE_PTP_FUP_SUBTLV_STATUS_LENGTH;
            }

            /* Update information for Userdata SubTLV */
            if (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV) {
                tlvLength = (uint16_t)(tlvLength + Score_PtpWriteSubTLVUserdata(subTLVPtr));
                subTLVPtr += SCORE_PTP_FUP_SUBTLV_USERDATA_LENGTH;
            }

            /* AutosarTlvType */
            autosarTlvPtr->tlvType = Score_PtpFlip16((uint16_t)(SCORE_PTP_FUP_AUTOSAR_TLV_TYPE));

            /* AUTOSAR OrganizationId 0x0080C2 [IEEE802.1AS] */
            autosarTlvPtr->orgId[0] = (SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID >> 16) & 0xFF;
            autosarTlvPtr->orgId[1] = (SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID >> 8) & 0xFF;
            autosarTlvPtr->orgId[2] = SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID & 0xFF;

            /* AUTOSAR organizationSub-Type */
            autosarTlvPtr->orgSubType[0] = (SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE >> 16) & 0xFF;
            autosarTlvPtr->orgSubType[1] = (SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE >> 8) & 0xFF;
            autosarTlvPtr->orgSubType[2] = SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE & 0xFF;

            /* AUTOSAR length field = 6 + totalSubtlvLength */
            tlvLength = (uint16_t)(tlvLength + (uint16_t)(SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID_LENGTH) +
                                    (uint16_t)(SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE_LENGTH));

            autosarTlvPtr->tlvLength = Score_PtpFlip16(tlvLength);

            memcpy((void *)autosarTlvBufferOut, (void *)autosarTlvPtr, (size_t)SCORE_PTP_FUP_AUTOSAR_TLV_INFO_LENGTH);
        }
    }
}

/** @brief Read the SubTlv TimeSecured information from the ptpd buffer.
 * @details
 * This function is used by Consumer to read the TimeSecured information from
 * incoming buffer.
 * @param[out]  timeSecuredIn   Pointer to timeSecured array.
 * @return int  Number of bytes read, -1 otherwise.
 */
static int Score_PtpReadSubTLVTimeSecured(const uint8_t *timeSecuredIn) {
    Score_PtpSubtlvTimeSecuredType timeSecured;
    unsigned int index = 0u;
    uint8_t dataID = 0u;
    uint8_t crcComputedTime0 = 0u;
    uint8_t crcComputedTime1 = 0u;
    uint8_t crcInputStream0[SCORE_PTP_TIMESECURED_CRC_TIME_0_STREAM_LENGTH];
    uint8_t crcInputStream1[SCORE_PTP_TIMESECURED_CRC_TIME_1_STREAM_LENGTH];
    uint8_t processPacket;

    memcpy((void *)(&timeSecured), (void *)(timeSecuredIn), sizeof(Score_PtpSubtlvTimeSecuredType));
    memset(crcInputStream0, 0u, SCORE_PTP_TIMESECURED_CRC_TIME_0_STREAM_LENGTH);
    memset(crcInputStream1, 0u, SCORE_PTP_TIMESECURED_CRC_TIME_1_STREAM_LENGTH);

    if (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV) {
        if (TSYNC_CRC_NOT_VALIDATED != ptpConfig.timebaseConfig.crcValidation) {
            if (SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED == timeSecured.subtlvType) {
                /* Update timeflags in crc stream */
                crcInputStream0[index++] = timeSecured.crcTimeFlags;

                /* Update the remaining configuration elements in crc stream */
                if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK)) {
                    /*[PRS_TS_00184] If applying the CRC calculation on multibyte message data, the
                    byte order shall be in ascending order of the octets, i.e., the octet with the lowest offset
                    shall be used first.*/
                    crcInputStream0[index++] = (ptpConfig.timebaseId & SCORE_PTP_LOWER_BYTE_MASK);
                    crcInputStream0[index++] = ((ptpConfig.timebaseId & SCORE_PTP_HIGHER_BYTE_MASK) >> 8);
                }
                if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK)) {
                    memcpy(&crcInputStream0[index], &consumerData.followUpMsg.followupHeaderInfo.sourcePortIdentity,
                           SCORE_PTP_FUP_MESSAGE_HEADER_PORT_IDENTITY_LENGTH);
                    index += SCORE_PTP_FUP_MESSAGE_HEADER_PORT_IDENTITY_LENGTH;
                }
                if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK)) {
                    memcpy(&crcInputStream0[index], &consumerData.followUpMsg.preciseOriginTimestamp,
                           SCORE_PTP_FUP_MESSAGE_PRECISE_ORIGIN_TIME_LENGTH);
                    index += SCORE_PTP_FUP_MESSAGE_PRECISE_ORIGIN_TIME_LENGTH;
                }
                /* Finally, update DataID in crc stream */
                dataID = Score_PtpGetDataId(consumerData.followUpMsg.followupHeaderInfo.sequenceId);
                crcInputStream0[index++] = dataID;

                /* Compute crc_time_0 now */
                /* Is the stream length always 5 or it depends on the configuration - currentlly it is taken based on
                 * configuration */
                crcComputedTime0 =
                    Crc_CalculateCRC8H2F((uint8_t *)(&crcInputStream0[0]), index, 0u, SCORE_PTP_TRUE);

                /* Reset the index for next stream */
                index = 0u;

                /* Update timeflags in crc stream */
                crcInputStream1[index++] = timeSecured.crcTimeFlags;

                if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK)) {
                    crcInputStream1[index++] =
                        (consumerData.followUpMsg.followupHeaderInfo.messageLength & SCORE_PTP_LOWER_BYTE_MASK);
                    crcInputStream1[index++] =
                        ((consumerData.followUpMsg.followupHeaderInfo.messageLength & SCORE_PTP_HIGHER_BYTE_MASK) >> 8);
                }
                if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK)) {
                    memcpy(&crcInputStream1[index], &consumerData.followUpMsg.followupHeaderInfo.correctionField,
                           SCORE_PTP_FUP_MESSAGE_HEADER_CORRECTION_FIELD_LENGTH);
                    index += SCORE_PTP_FUP_MESSAGE_HEADER_CORRECTION_FIELD_LENGTH;
                }
                if (SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK)) {
                    crcInputStream1[index++] =
                        (consumerData.followUpMsg.followupHeaderInfo.sequenceId & SCORE_PTP_LOWER_BYTE_MASK);
                    crcInputStream1[index++] =
                        ((consumerData.followUpMsg.followupHeaderInfo.sequenceId & SCORE_PTP_HIGHER_BYTE_MASK) >> 8);
                }

                /* Finally, update DataID in crc stream */
                crcInputStream1[index++] = dataID;

                /* Compute crc_time_1 now */
                /* Is the stream length always 5 or it depends on the configuration - currentlly it is taken based on
                 * configuration */
                crcComputedTime1 =
                    Crc_CalculateCRC8H2F((uint8_t *)(&crcInputStream1[0]), index, 0u, SCORE_PTP_TRUE);

                if ((crcComputedTime0 == timeSecured.crcTime0) && (crcComputedTime1 == timeSecured.crcTime1)) {
                    processPacket = SCORE_PTP_TRUE;
                } else {
                    processPacket = SCORE_PTP_FALSE;
                    printf(
                        "\n[INFO] score_aptpd2 : SubTLV TimeSecured has CRC inconsistencies [Incorrect CRC_Time_0 or CRC_Time_1].\n");
                }

            } else {
                processPacket = SCORE_PTP_FALSE;
                printf(
                    "[WARNING] score_aptpd2 : SubTLV TimeSecured has incorrect TLVType(%X). SubTLV TimeSecured dropped. \n", timeSecured.subtlvType);
            }
        } else {
            processPacket = SCORE_PTP_FALSE;
            printf(
                "[WARNING] score_aptpd2 : SubTLV TimeSecured is configured but the configuration parameter 'crcValidation' is not enabled. Please enable crcValidation inorder to receive TimeSecured SubTLV. \n");
        }

        /* Process packet only if preconditions are met */
        if (SCORE_PTP_TRUE == processPacket) {
            return sizeof(Score_PtpSubtlvTimeSecuredType);
        } else {
            return -1;
        }
    } else {
        /* Skip(Seeking) the bytes if the timesecured is present in the packet */
        if (SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED == timeSecured.subtlvType) {
            return sizeof(Score_PtpSubtlvTimeSecuredType);
        }
        else {
            return 0;
        }
    }
}

/** @brief Read the SubTLV userdata information from the ptpd buffer.
 * @details
 * This function is used by Consumer to read the userdata information from
 * incoming buffer.
 * @param[out]  userdataIn   Pointer to userdata array.
 * @return int  Number of bytes read, -1 otherwise.
 */
static int Score_PtpReadSubTLVUserdata(const uint8_t *userdataIn) {
    Score_PtpSubtlvUserdataType userdata;
    uint8_t crcInputStream[SCORE_PTP_USERDATA_CRC_STREAM_LENGTH];
    uint8_t crcComputed = 0u;
    uint8_t processPacket;

    memcpy((void *)(&userdata), (void *)(userdataIn), sizeof(Score_PtpSubtlvUserdataType));
    memset(crcInputStream, 0u, SCORE_PTP_USERDATA_CRC_STREAM_LENGTH);

    if (TSYNC_SUBTLV_SUPPORTED == ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV) {
        if (SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC == userdata.subtlvType) {
            /* TSYNC_CRC_NOT_VALIDATED - CRC enabled follow-up messages must to be discarded */
            if (TSYNC_CRC_NOT_VALIDATED == ptpConfig.timebaseConfig.crcValidation) {
                processPacket = SCORE_PTP_FALSE;
            }
            /* TSYNC_CRC_IGNORED - Dont care for the CRC Validation, just process the SubTLV */
            else if (TSYNC_CRC_IGNORED == ptpConfig.timebaseConfig.crcValidation) {
                processPacket = SCORE_PTP_TRUE;
            }
            /* TSYNC_CRC_OPTIONAL  - Go for CRC Validation as the packet received is CRC enabled
               TSYNC_CRC_VALIDATED - CRC Validation is mandatory */
            else if ((TSYNC_CRC_OPTIONAL == ptpConfig.timebaseConfig.crcValidation) ||
                     (TSYNC_CRC_VALIDATED == ptpConfig.timebaseConfig.crcValidation)) {
                crcInputStream[0] = userdata.userdataLen;
                crcInputStream[1] = userdata.userdataByte0;
                crcInputStream[2] = userdata.userdataByte1;
                crcInputStream[3] = userdata.userdataByte2;
                if (ptpConfig.timebaseConfig.numFupDataIdEntries > 0u) {
                    crcInputStream[4] = Score_PtpGetDataId(consumerData.followUpMsg.followupHeaderInfo.sequenceId);
                }

                crcComputed = Crc_CalculateCRC8H2F((uint8_t *)(&crcInputStream[0]), SCORE_PTP_USERDATA_CRC_STREAM_LENGTH,
                                                   0u, SCORE_PTP_TRUE);

                if (crcComputed == userdata.crcUserdata) {
                    processPacket = SCORE_PTP_TRUE;
                } else {
                    processPacket = SCORE_PTP_FALSE;
                    printf("\n[INFO] score_aptpd2 : SubTLV Userdata has CRC inconsistencies [Incorrect CRC].\n");
                }
            } else {
                processPacket = SCORE_PTP_FALSE;
                printf("[INFO] score_aptpd2 : Unknown crcValidation [%d] configured for the domain-id [%d]\n",
                       ptpConfig.timebaseConfig.crcValidation, ptpConfig.timebaseId);
            }
        } else if (SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC == userdata.subtlvType) {
            if (TSYNC_CRC_VALIDATED == ptpConfig.timebaseConfig.crcValidation) {
                processPacket = SCORE_PTP_FALSE;
            } else {
                processPacket = SCORE_PTP_TRUE;
            }
        } else {
            processPacket = SCORE_PTP_FALSE;
            printf("[WARNING] score_aptpd2 : SubTLV Userdata has incorrect TLVType(%X). SubTLV Userdata dropped. \n",
                   userdata.subtlvType);
        }

        if (SCORE_PTP_TRUE == processPacket) {
            consumerData.followUpMsg.subTlv.userdata.userDataLength = userdata.userdataLen;
            consumerData.followUpMsg.subTlv.userdata.userByte0 = userdata.userdataByte0;
            consumerData.followUpMsg.subTlv.userdata.userByte1 = userdata.userdataByte1;
            consumerData.followUpMsg.subTlv.userdata.userByte2 = userdata.userdataByte2;

            return sizeof(Score_PtpSubtlvUserdataType);
        } else {
            return -1;
        }
    } else {
        /* Skip(Seeking) the bytes if the userdata is present in the packet */
        if ((SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC == userdata.subtlvType) ||
            (SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC == userdata.subtlvType)) {
            return sizeof(Score_PtpSubtlvUserdataType);
        }
        else {
            return 0;
        }
    }
}

/** @brief Read the SubTLV status information from the ptpd buffer.
 * @details
 * This function is used by Consumer to read the status information from
 * incoming buffer.
 * @param[out]  statusSubTlvIn   Pointer to status array.
 * @return int  Number of bytes read, -1 otherwise.
 */
static int Score_PtpReadSubTLVStatus(const uint8_t *statusSubTlvIn) {

    Score_PtpSubtlvStatusType statusSubTlv;

    memset((void *)(&statusSubTlv), 0u, sizeof(Score_PtpSubtlvStatusType));
    memcpy((void *)(&statusSubTlv), (void *)(statusSubTlvIn), sizeof(Score_PtpSubtlvStatusType));

    /* Gateway scenario is not yet supported by Adaptive Autosar.
    Since Status SubTLV covers gateway scenario, we ignore any StatusTLV that is received */
    if ((SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC == statusSubTlv.subtlvType) || (SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITHOUT_CRC == statusSubTlv.subtlvType )) {
        printf("[INFO] score_aptpd2 : Received a unsupported SubTLV - Status.Ignoring the contents of the SubTlv and processing the followup message.\n");
        return sizeof(Score_PtpSubtlvStatusType);
    }

    return 0;
}

/** @brief Reads the Autosar TLV information from the ptpd buffer.
 * @details
 * This function is used by Consumer to read/extract the TLV from the
 * incoming buffer.
 * @param[in]  autosarTlvBufferIn   Pointer to buffer where Autosar Tlv information
 * can be read from.
 * @return TRUE if Autosar SubTLVs were read successfully, FALSE otherwise
 */
Score_PtpBooleanType Score_PtpReadSubTlvFromBuffer(const uint8_t *autosarTlvBufferIn) {
    Score_PtpAutosarTlvType *autosarTlvPtr = NULL;
    const uint8_t *subTLVPtr = NULL;
    int subTlvLength = 0;
    uint16_t expectedLenField = (uint16_t)(ptpConfig.configGlobals.configSubTlvLength + (uint16_t)(SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID_LENGTH) +
                                           (uint16_t)(SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE_LENGTH));

    /* Read SubTLV from incoming buffer only if message compliance is TSYNC_MC_IEEE8021AS_AUTOSAR */
    if (TSYNC_MC_IEEE8021AS_AUTOSAR == ptpConfig.timebaseConfig.messageCompliance) {
        if (NULL != autosarTlvBufferIn) {
            autosarTlvPtr = (Score_PtpAutosarTlvType *)(autosarTlvBufferIn);

            /* Check if any SubTLVs are configured */
            if (0u < ptpConfig.configGlobals.configSubTlvLength) {
                /* We are expecting ATLEAST our configured SubTLVs to be available in the followup packet */
                if (expectedLenField <= Score_PtpFlip16(autosarTlvPtr->tlvLength)) {
                    subTLVPtr = autosarTlvBufferIn + SCORE_PTP_FUP_AUTOSAR_TLV_INFO_LENGTH;
                    /* Update information for TimeSecured SubTLV */
                    subTlvLength = Score_PtpReadSubTLVTimeSecured(subTLVPtr);

                    if (0 <= subTlvLength) {
                        subTLVPtr += subTlvLength;
                    } else {
                        /* Entire Follow-up message must be dropped when CRC validation(within SubTLVs) was not
                         * successful */
                        return SCORE_PTP_FALSE;
                    }

                    /* We dont support status SubTLV, this only serves as default handling */
                    subTLVPtr += Score_PtpReadSubTLVStatus(subTLVPtr);

                    /* Update information for Userdata SubTLV */
                    subTlvLength = Score_PtpReadSubTLVUserdata(subTLVPtr);
                    if (0 <= subTlvLength) {
                        subTLVPtr += subTlvLength;
                    } else {
                        /* Entire Follow-up message must be dropped when CRC validation(within SubTLVs) was not
                         * successful */
                        return SCORE_PTP_FALSE;
                    }
                } else {
                    printf(
                        "[WARNING] aptpd2: Dropped the autosar TLV as the SubTLVs received do not match the configured SubTLVs.\n");
                        return SCORE_PTP_FALSE;
                }
            } else {
                /* The consumer has no SubTLV configured. Proceed further to process only origin time */
                return SCORE_PTP_TRUE;
            }
        }
    }
    /* Control reaches here only when the subtlv parsing is successful. Hence packet does not get dropped */
    return SCORE_PTP_TRUE;
}
