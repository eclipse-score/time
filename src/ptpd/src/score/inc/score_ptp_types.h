/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_TYPES_H
#define SCORE_PTP_TYPES_H

#include <stdint.h>
#include "tsync_ptp_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Macro to checks if a specific CRC flag in a variable is set.
 */
#define SCORE_PTP_CHECK_CRC_FLAG(variable, bitmask) ((variable & bitmask) == bitmask)

/** @brief Macro to set a bit in CRC time flags, in the given variable
 */
#define SCORE_PTP_SET_CRC_FLAG(variable, bitmask) (variable |= bitmask)

/**
 * @brief Macro to define length of port number(to store PortIdentity)
 */
#define SCORE_PTP_PORT_NUMBER_LENGTH 2u

/**
 * @brief Macro to define length of clock identity(to store PortIdentity)
 */
#define SCORE_PTP_CLOCK_IDENTITY_LENGTH 8u

/**
 * @brief Macro to define length of port identity
 */
#define SCORE_PTP_FUP_MESSAGE_HEADER_PORT_IDENTITY_LENGTH (SCORE_PTP_CLOCK_IDENTITY_LENGTH + SCORE_PTP_PORT_NUMBER_LENGTH)

/**
 * @brief Macro to define length of precise origin time
 */
#define SCORE_PTP_FUP_MESSAGE_PRECISE_ORIGIN_TIME_LENGTH 10u

/**
 * @brief Macro to define length of domain number(timebase id)
 */
#define SCORE_PTP_FUP_MESSAGE_HEADER_DOMAIN_NUMBER_LENGTH 2u

/**
 * @brief Macro to define length of message length field
 */
#define SCORE_PTP_FUP_MESSAGE_HEADER_MESSAGE_LENGTH_BYTES 2u

/**
 * @brief Macro to define length of Correction field inside the followup header
 */
#define SCORE_PTP_FUP_MESSAGE_HEADER_CORRECTION_FIELD_LENGTH 8u

/**
 * @brief Macro to define length of Sequence ID inside the followup header
 */
#define SCORE_PTP_FUP_MESSAGE_HEADER_SEQUENCE_ID_LENGTH 2u

/**
 * @brief Macro to define indices of various fields in Followup message
 */
#define SCORE_PTP_FUP_MSG_TYPE_IDX 0u
#define SCORE_PTP_FUP_MSG_LENGTH_IDX 2u
#define SCORE_PTP_FUP_FLAGS_OCTET_0_IDX 6u
#define SCORE_PTP_FUP_FLAGS_OCTET_1_IDX 7u
#define SCORE_PTP_FUP_SEQUENCE_ID_IDX 30u
#define SCORE_PTP_FUP_CONTROL_IDX 32u
#define SCORE_PTP_FUP_LOG_MSG_INTERVAL_IDX 33u
#define SCORE_PTP_FUP_ORIGIN_TIME_SECONDSHI_IDX 34u
#define SCORE_PTP_FUP_ORIGIN_TIME_SECONDS_IDX 36u
#define SCORE_PTP_FUP_ORIGIN_TIME_NANOSECONDS_IDX 40u
#define SCORE_PTP_FUP_INFO_START_IDX 44u
#define SCORE_PTP_FUP_INFO_AUTOSAR_START_IDX 76u

/**
 * @brief Macros to define SubTlv type for each SubTLVs */
#define SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC 0x61
#define SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC 0x60
#define SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED 0x28
#define SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC 0x50
#define SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITHOUT_CRC 0x51

/**
 * @brief Subtlv length field for UserData - user bytes + crc */
#define SCORE_PTP_FUP_SUBTLV_LEN_FIELD_USERDATA 5u

/**
 * @brief Subtlv length field for TimeSecured - CRC_Time_Flags + CRC_Time_0 + CRC_Time_1 */
#define SCORE_PTP_FUP_SUBTLV_LEN_FIELD_TIMESECURED 3u

/**
 * @brief Macros to define SubTLV length in bytes for each SubTLV message
 */
#define SCORE_PTP_FUP_SUBTLV_TIMESECURED_LENGTH 5u
#define SCORE_PTP_FUP_SUBTLV_STATUS_LENGTH 4u
#define SCORE_PTP_FUP_SUBTLV_USERDATA_LENGTH 7u
#define SCORE_PTP_FUP_SUBTLV_OFS_LENGTH 19u
#define SCORE_PTP_TOTAL_SUBTLV_LENGTH                                                                                \
    (SCORE_PTP_FUP_SUBTLV_TIMESECURED_LENGTH + SCORE_PTP_FUP_SUBTLV_STATUS_LENGTH + SCORE_PTP_FUP_SUBTLV_USERDATA_LENGTH + \
     SCORE_PTP_FUP_SUBTLV_OFS_LENGTH)
/**
 * @brief Total Followup TLV information (IEEE + AUTOSAR ) = 79 bytes
 *        FollowUp Tlv info from IEEE           = 32 bytes
 *        Followup Tlv info from AUOSAR         = 10 bytes
 *        Followup SubTlv from AUTOSAR          = 37 bytes
 */
#define SCORE_PTP_FUP_INFO_TLV_LENGTH 79u

/**
 * @brief Macros for Follow-Up information according to IEEE 802.1AS
 */
#define SCORE_PTP_FUP_TLV_TYPE 0x0003u
#define SCORE_PTP_FUP_TLV_LENGTH 0x001Cu /* 28u */
#define SCORE_PTP_FUP_TLV_ORGAID 0x0080C2u
#define SCORE_PTP_FUP_TLV_ORGASUBTYPE 0x00001u
#define SCORE_PTP_FUP_TLV_SCALEDRATEOFF 0x00000000u
#define SCORE_PTP_FUP_TLV_GMTIMEBASEIND 0x0000u
#define SCORE_PTP_FUP_TLV_LASTGMPHASECHG 0x00u
#define SCORE_PTP_FUP_TLV_LASTGMFREQCHG 0x000000u

/** @brief Macros for Follow-Up information according to AUTOSAR
 */
#define SCORE_PTP_FUP_AUTOSAR_TLV_TYPE 0x0003u
#define SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID 0x1A75FB     /* AUTOSAR */
#define SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE 0x605676 /* BCD coded GlobalTimeEthTSyn */

/** @brief Macros related to AUTOSAR TLV
 */
#define SCORE_PTP_FUP_AUTOSAR_TLV_INFO_LENGTH 10u
#define SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH 45u

/** @brief Macros to define size of the octet for the AUTOSAR TLVs
 */
#define SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID_LENGTH 3u     /* Length of ORGAID */
#define SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE_LENGTH 3u /* Length of ORGASUBTYPE */

/** @brief Magic Number macro for Nanoseconds */
#define SCORE_PTP_NANOSEC 1000000000UL

/** @brief Max Value of Nanoseconds */
#define SCORE_PTP_NS_UPPERLIMIT 0x3B9AC9FFU

/** @brief Mask for extracting lower 32bits from uint64_t variable */
#define SCORE_PTP_LOWER_DOUBLE_WORD_MASK 0x00000000FFFFFFFFUL

/** @brief Mask for extracting upper 32bits from uint64_t variable */
#define SCORE_PTP_HIGHER_DOUBLE_WORD_MASK 0xFFFFFFFF00000000UL

/** @brief Mask for extracting lower 8bits from uint16_t variable */
#define SCORE_PTP_LOWER_BYTE_MASK 0x00FFu

/** @brief Mask for extracting upper 8bits from uint16_t variable */
#define SCORE_PTP_HIGHER_BYTE_MASK 0xFF00u

/** @brief Macro to for sign of Timestamp - 1 means POSITIVE timestamp */
#define SCORE_PTP_POSITIVE 1u

/** @brief Macro to for sign of Timestamp - 0 means NEGATIVE timestamp */
#define SCORE_PTP_NEGATIVE 0u

/** @brief Macro for FALSE */
#define SCORE_PTP_FALSE 0u

/** @brief Macro for TRUE */
#define SCORE_PTP_TRUE (!SCORE_PTP_FALSE)


/** @brief FollowUp message length when message compliance is set to
 * IEEE8021AS_AUTOSAR ( The packet will include AUTOSAR SubTlvs).
 * Length here includes only AUTOSAR Header (excluding SubTLVs)
 * The SubTLVs length have to be individually accumulated depending on
 * which of them is configured.
 */
#define SCORE_PTP_FUP_PACKET_LENGTH_FIXED_AUTOSAR 86u

/** @brief FollowUp message length when message compliance is set to
 * IEEE8021AS
 */
#define SCORE_PTP_FUP_PACKET_LENGTH_IEEE8021AS 76u

/**
 * @brief Length of the input stream for computing crc of userdata
 * CRC shall be computed using - UserDataLength, UserByte_0, UserByte_1, UserByte_2 and DataID
 */
#define SCORE_PTP_USERDATA_CRC_STREAM_LENGTH 5u

/**
 * @brief Length of the input stream for computing crc of timeSecured subtlv
 * CRC shall be computed using - CRC_Time_Flags, domainNumber/messageLength,
 * sourcePortIdentity/correctionField, preciseOriginTimestamp/sequenceId and DataID
 */
#define SCORE_PTP_TIMESECURED_CRC_TIME_0_STREAM_LENGTH                                                    \
    (SCORE_PTP_FUP_MESSAGE_HEADER_DOMAIN_NUMBER_LENGTH + SCORE_PTP_FUP_MESSAGE_HEADER_PORT_IDENTITY_LENGTH + \
     SCORE_PTP_FUP_MESSAGE_PRECISE_ORIGIN_TIME_LENGTH)

/**
 * @brief Length of the input stream for computing crc of timeSecured subtlv
 * CRC shall be computed using - CRC_Time_Flags, domainNumber/messageLength,
 * sourcePortIdentity/correctionField, preciseOriginTimestamp/sequenceId and DataID
 */
#define SCORE_PTP_TIMESECURED_CRC_TIME_1_STREAM_LENGTH                                                       \
    (SCORE_PTP_FUP_MESSAGE_HEADER_MESSAGE_LENGTH_BYTES + SCORE_PTP_FUP_MESSAGE_HEADER_CORRECTION_FIELD_LENGTH + \
     SCORE_PTP_FUP_MESSAGE_HEADER_SEQUENCE_ID_LENGTH)

/**
 * @brief Bit Masks for CRC Time Flags
 */
#define SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK 0x01
#define SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK 0x02
#define SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK 0x04
#define SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK 0x08
#define SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK 0x10
#define SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK 0x40

/**
 * @brief Number of Milliseconds in one sec
 */
#define SCORE_PTP_DEFAULT_SYNC_PERIOD 1000

/** @brief Typedef to define sign of a number */
typedef uint8_t Score_PtpSignType;

/** @brief Typedef to define boolean type */
typedef uint32_t Score_PtpBooleanType;

/** @brief The return type for the Ptpd APIs */
typedef enum { SCORE_PTP_E_OK = 0, SCORE_PTP_E_NOT_OK = 1 } Score_PtpReturnType;

/** @brief Enum to identify the message type - Consumer */
typedef enum {SCORE_PTP_INGRESS_UNDEFINED = -1, SCORE_PTP_INGRESS_T1 = 0, SCORE_PTP_INGRESS_T2 = 1 } Score_PtpIngressVltType;

/** @brief Enum to hold the current State of the Sync-Cycle */
typedef enum { SCORE_PTP_STATE_SYNC_RCVD = 0, SCORE_PTP_STATE_FUP_RCVD, SCORE_PTP_STATE_INVALID } Score_PtpStateType;

/** @brief Enum to identify the message type - Provider */
typedef enum { SCORE_PTP_EGRESS_T0 = 0, SCORE_PTP_EGRESS_T4 = 1 } Score_PtpEgressVltType;

/** @brief Defines the type for ClockIdentity
 */
typedef char Score_PtpClockIdentityType[SCORE_PTP_CLOCK_IDENTITY_LENGTH];

/** @brief Defines the structure for PortIdentity
 */
typedef struct {
    Score_PtpClockIdentityType clockIdentity;
    uint16_t portNumber;
} Score_PtpPortIdentity;

#pragma pack(push, 1)

/** @brief Defines the followup header structure
 */
typedef struct {
    uint8_t transportSpecific;
    uint8_t messageType;
    uint8_t reserved0;
    uint8_t versionPtp;
    uint16_t messageLength;
    uint8_t domainNumber;
    uint8_t reserved1;
    uint8_t flagField0;
    uint8_t flagField1;
    uint64_t correctionField;
    uint32_t reserved2;
    Score_PtpPortIdentity sourcePortIdentity;
    uint16_t sequenceId;
    uint8_t controlField;
    uint8_t logMessageInterval;
} Score_PtpFupHeaderType;

/** @brief Defines the TimeSecured SubTLV message format
 */
typedef struct {
    uint8_t subtlvType;   /* Type of the Subtlv */
    uint8_t subtlvLength; /* Length of the Subtlv */
    uint8_t crcTimeFlags; /* Bitmask indicating the followup messsage fields that are used for CRC_Time_0 and CRC_Time_1
                             calculation */
    uint8_t crcTime0;     /* Userdata byte 0*/
    uint8_t crcTime1;     /* Userdata byte 1*/
} Score_PtpSubtlvTimeSecuredType;

/** @brief Defines the Status (Secured and NonSecured) SubTLV message format
 */
typedef struct {
    uint8_t subtlvType;   /* Type of the Subtlv */
    uint8_t subtlvLength; /* Length of the Subtlv */
    uint8_t status;       /* Timebase status */
    uint8_t crcStatus;    /* CRC for the Status byte, not used for Status Not-Secured */
} Score_PtpSubtlvStatusType;

/** @brief Defines the Userdata (Secured and NonSecured) SubTLV message format
 */
typedef struct {
    uint8_t subtlvType;    /* Type of the Subtlv */
    uint8_t subtlvLength;  /* Length of the Subtlv */
    uint8_t userdataLen;   /* Length of the SubtlvType - Userdata length */
    uint8_t userdataByte0; /* Userdata byte 0*/
    uint8_t userdataByte1; /* Userdata byte 1*/
    uint8_t userdataByte2; /* Userdata byte 2*/
    uint8_t crcUserdata;   /* CRC for the Userdata bytes, not used for Userdata Not-Secured*/
} Score_PtpSubtlvUserdataType;

/** @brief Defines the OFS data format in the Autosar SubTLV
 */
typedef struct {
    /*** Todo: the exact data structure format to be decided during the feature implementation */
    uint8_t subtlvType;     /* Type of the Subtlv */
    uint8_t subtlvLength;   /* Length of the Subtlv */
    uint8_t OfsTimeDomain;  /* Timedomain */
    uint8_t OfsTimeSec[6];  /* Seconds */
    uint8_t OfsTimeNSec[4]; /* Nanoseconds */
    uint8_t status;         /* Status of the timebase */
    uint8_t userdataLen;    /* Length of the SubtlvType - Userdata length */
    uint8_t userdataByte0;  /* Userdata byte 0*/
    uint8_t userdataByte1;  /* Userdata byte 1*/
    uint8_t userdataByte2;  /* Userdata byte 2*/
    uint8_t crcUserdata;    /* CRC for the Userdata bytes, not used for Userdata Not-Secured*/
} Score_PtpSubtlvOfsType;

/** @brief Defines structure of Autosar TLV (including SubTLV)
 */
typedef struct {
    /* AUTOSAR related */
    uint16_t tlvType;                                             /* AUTOSAR TLV type */
    uint16_t tlvLength;                                           /* AUTOSAR TLV length */
    uint8_t orgId[SCORE_PTP_FUP_AUTOSAR_TLV_ORGAID_LENGTH];          /* AUTOSAR Org Id*/
    uint8_t orgSubType[SCORE_PTP_FUP_AUTOSAR_TLV_ORGSUBTYPE_LENGTH]; /* AUTOSAR Org Sub Id*/
    Score_PtpSubtlvTimeSecuredType subTLVTimeSecured;
    Score_PtpSubtlvStatusType subTLVStatus;
    Score_PtpSubtlvUserdataType subTLVUserdata;
    Score_PtpSubtlvOfsType subTLVOFS;
} Score_PtpAutosarTlvType;

/** @brief Defines follow-up information structure as described in PTP
 * protocol. It contains only the fup information for IEEE and AUTOSAR.
 * The Protocol header is not included here as it is already filled by ptpd.
 */
typedef struct {
    uint16_t tlvType;      /* IEEE TLV type */
    uint16_t tlvLength;    /* IEEE TLV type */
    uint8_t orgId[3];      /* IEEE Org Id*/
    uint8_t orgSubType[3]; /* IEEE Org Sub Id*/
    uint32_t scaledrateOffset;
    uint16_t gmTimeBaseInd;
    uint8_t lastGmPhaseChange[12];
    uint32_t lastGmFreqChange;
    uint8_t autosarTlv[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
} Score_PtpProtocolFupInfoType;

#pragma pack(pop)

/** @brief Type for expressing timestamps along with the sign */
typedef struct {
    TSync_TimeStampType ts; /** TimeStamp value */
    Score_PtpSignType sign;    /** Positive (True) / negative (False) time */
} Score_PtpSignedTimeStampType;

/** @brief Defines various SubTlvs that are supported by AUTOSAR
 */
typedef struct {
    TSync_UserDataType userdata;
} Score_PtpSubTlvType;

/** @brief Defines the Follow_Up message fields that are required for the
 * calculation of globaltime
 */
typedef struct {
    Score_PtpFupHeaderType followupHeaderInfo;           /* Followup Header as received in follow-up message */
    Score_PtpSignedTimeStampType preciseOriginTimestamp; /* Sync Origin timestamp as received in the follow-up message */
    Score_PtpSubTlvType subTlv;                          /* SubTLV information */
} Score_PtpFupMessageType;

/** @brief Defines the all the received information - both Sync and Fup message
 * fields that are required for the calculation of globaltime
 */
typedef struct {
    Score_PtpFupMessageType followUpMsg;          /* Stores the follow-up message information */
    Score_PtpSignedTimeStampType correctionField; /* The received correction field converted to timestamp */
    TSync_VirtualLocalTimeType t1Vlt;          /* Stores the Ingress Time Stamp of Sync message - T1 */
    TSync_VirtualLocalTimeType t2Vlt;          /* Stores the Ingress Time Stamp of FUP message - T2 */
    Score_PtpStateType syncState;                 /* The current State of the Sync Cycle */
} Score_PtpConsumerDataType;

/** @brief Defines the Timestamp type compatible to PTP daemon.
 */
typedef struct {
    uint32_t seconds;     /* 32 bit LSB of the 48 bits Seconds part of the time */
    uint16_t secondsHi;   /* 16 bit MSB of the 48 bits Seconds part of the time */
    uint32_t nanoseconds; /* nanoseconds part of the time */
} Score_PtpTimeStampType;

/** @brief Defines all the Provider information needed for the calculation of
 * preciseOriginTime
 */
typedef struct {
    Score_PtpFupMessageType followUpMsg; /* Stores the follow-up message information */
    TSync_VirtualLocalTimeType t0Vlt; /* Stores the Egress Virtual Local Time of Sync message - T0 */
    TSync_VirtualLocalTimeType t4Vlt; /* Stores the Egress Virtual Local Time when Sync message was on the Bus - T4 */
    TSync_TimeStampType t0GlobalTime; /* Stores the Egress Global Time of Sync message - T0 */
    volatile Score_PtpBooleanType immediateTimeTransmission; /* Indicates if ImmediateTimeTransmission is needed. TRUE = Transmit immediately,
                                      FALSE = No immediate transmission requested */
} Score_PtpProviderDataType;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCORE_PTP_TYPES_H */
