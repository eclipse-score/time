/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef TSYNC_PTP_LIB_TYPES_H_INCLUDED
#define TSYNC_PTP_LIB_TYPES_H_INCLUDED

// coverity[autosar_cpp14_a1_1_1_violation] This library provides a C API, the use of C headers is acceptable.
#include <stdint.h>

/** @brief The return type for the public API */
typedef uint8_t TSync_ReturnType;

/** @brief return value indicating success */
#define E_OK 0x00U

/** @brief return value indicating failure */
#define E_NOT_OK 0x01U

/** @brief Timebase identifier */
typedef uint16_t TSync_SynchronizedTimeBaseType;

/** @brief Callback type for TransmitGlobalTime */
typedef TSync_ReturnType (*TSync_TransmitGlobalTimeCallback)(TSync_SynchronizedTimeBaseType);

/** @brief Reflects the state of a timebase */
typedef uint8_t TSync_TimeBaseStatusType;

/** @brief Reflects an invalid handle */
#ifdef __cplusplus
#define TSYNC_INVALID_HANDLE (static_cast<TSync_TimeBaseHandleType*>(0))
#else
#define TSYNC_INVALID_HANDLE ((TSync_TimeBaseHandleType*)0)
#endif

/** @brief Reflects an invalid time base id */
#define TSYNC_INVALID_TIME_BASE_ID 0xffffu

/** @brief Timebase handle type as returned by TSync_OpenTimebase() */
typedef void* TSync_TimeBaseHandleType;

/** @brief Indicates a timeout has happened on receiving a synchronization
 * message */
#define TIMEBASE_STATUS_BIT_TIMEOUT 0u

/** @brief Indicates that the local timebase is synced to a gateway and not
 * the global time master */
#define TIMEBASE_STATUS_BIT_SYNC_TO_GATEWAY 2u

/** @brief Indicates that the local timebase was synced at least once with
 * the global timebase */
#define TIMEBASE_STATUS_BIT_GLOBAL_SYNC 3u

/** @brief Indicates that a leap into the future of a received timebase
 * exceeded a configured threshold */
#define TIMEBASE_STATUS_BIT_TIMELEAP_FUTURE 4u

/** @brief Indicates that a leap into the past of a received timebase
 * exceeded a configured threshold */
#define TIMEBASE_STATUS_BIT_TIMELEAP_PAST 5u

/** @brief The size of the string holding the zero terminated instance
 * specifier name */
#define TSYNC_INSTANCE_SPECIFIER_MAX_SIZE 256U

/* TODO: investigate possible packing/alignment issues.
   TODO:  #pragma pack(push, 8) might be a better choice here. */
#pragma pack(push, 1)

/** @brief Type for expressing timestamps in relative in absolute time */
// clang-format off
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "This header offers a C API, so struct members cannot be initialized", true)
typedef struct {
    // clang-format on
    /** @brief Status indicator bitfield of the timestamp */
    TSync_TimeBaseStatusType timeBaseStatus;
    /** @brief nanoseconds part of the time */
    uint32_t nanoseconds;
    /** @brief 32 bit LSB of the 48 bits Seconds part of the time */
    uint32_t seconds;
    /** @brief 16 bit MSB of the 48 bits Seconds part of the time */
    uint16_t secondsHi;
} TSync_TimeStampType;

/** @brief Current user data of the Time Base */
typedef struct {
    /** @brief User Data Length in bytes, value range: 0..3 */
    uint8_t userDataLength;
    /** @brief User Byte 0 */
    uint8_t userByte0;
    /** @brief User Byte 1 */
    uint8_t userByte1;
    /** @brief User Byte 2 */
    uint8_t userByte2;
} TSync_UserDataType;

/** @brief Structure which contains additional measurement data */
typedef struct {
    /** @brief Propagation delay in nanoseconds */
    uint32_t pathDelay;
} TSync_MeasurementType;

/** @brief Time stamps of the Virtual Local Time. The unit is nanoseconds */
typedef struct {
    /** @brief Least significant 32 bits of the 64 bit Virtual Local Time */
    uint32_t nanosecondsLo;
    /** @brief Most significant 32 bits of the 64 bit Virtual Local Time */
    uint32_t nanosecondsHi;
} TSync_VirtualLocalTimeType;

/** @brief This structure provides a mapping of timebase identifiers to
 * instance specifiers. */
typedef struct {
    /** @brief A timebase identifier */
    TSync_SynchronizedTimeBaseType timeBaseId;
    /** @brief The corresponding instance specifier */
    char instanceSpecifier[TSYNC_INSTANCE_SPECIFIER_MAX_SIZE + 1];

} TSync_TimeBaseIdentifierMappingType;

/** @brief This structure describes the basic layout of the memory block
 * holding the identifier mappings.
 */
typedef struct {
    /** @brief The number of mappings will be the first 16Bit value in shared
     * memmory */
    uint16_t numMappings;
    /** @brief The data following right after the numMappings entry will be
     * interpreted as the beginning of the mappings array.
     */
    TSync_TimeBaseIdentifierMappingType mappings[1];
} TSync_TimeBaseIdentifierMappingLayoutType;
#pragma pack(pop)

/* Configuration data types */

/** @brief This enum defines the different message compliance variants. */
typedef enum { TSYNC_MC_IEEE8021AS = 0, TSYNC_MC_IEEE8021AS_AUTOSAR } TSync_MessageCompliance;

/** @brief This enum defines the different CRC support variants for time providers. */
typedef enum { TSYNC_CRC_NOT_SUPPORTED = 0, TSYNC_CRC_SUPPORTED } TSync_CrcSupport;

/** @brief This enum defines the different CRC validation variants for time consumers. */
typedef enum {
    TSYNC_CRC_VALIDATED = 0,
    TSYNC_CRC_NOT_VALIDATED,
    TSYNC_CRC_IGNORED,
    TSYNC_CRC_OPTIONAL

} TSync_RxCrcValidation;

/** @brief This enum defines the possible ptpd roles. */
typedef enum { TSYNC_ROLE_INVALID = 0, TSYNC_ROLE_MASTER, TSYNC_ROLE_SLAVE } TSync_Role;

/** @brief This enum defines the different SubTLV support options */
typedef enum { TSYNC_SUBTLV_NOT_SUPPORTED = 0, TSYNC_SUBTLV_SUPPORTED } TSync_SubTLVSupport;

#define TSYNC_MAX_FUP_DATA_ID_ENTRIES 16
#define TSYNC_MAC_ADDRESS_STRING_LENGTH 12

/** @brief This struct holds the SubTLV configuration. */
typedef struct {
    /** @brief holds configured Time SubTLV support as represented by the TSync_SubTLVSupport enum */
    uint32_t followUpTimeSubTLV;
    /** @brief holds configured Status SubTLV support as represented by the TSync_SubTLVSupport enum */
    uint32_t followUpStatusSubTLV;
    /** @brief holds configured Userdata SubTLV support as represented by the TSync_SubTLVSupport enum */
    uint32_t followUpUserDataSubTLV;
} TSync_SubTlvConfig;

/** @brief This struct holds the CRC flags for PTP network message fields. */
typedef struct {
    /** @brief flag indicating whether the correction field will be considered
     * for CRC calculation as represented by the TSync_CrcSupport enum */
    uint32_t correction_field;
    /** @brief flag indicating whether the domain number field will be considered
     * for CRC calculation as represented by the TSync_CrcSupport enum */
    uint32_t domain_number;
    /** @brief flag indicating whether the message length field will be considered
     * for CRC calculation as represented by the TSync_CrcSupport enum */
    uint32_t message_length;
    /** @brief flag indicating whether the precide origin timestamp field will be considered
     * for CRC calculation as represented by the TSync_CrcSupport enum */
    uint32_t precise_origin_timestamp;
    /** @brief flag indicating whether the sequence id field will be considered
     * for CRC calculation as represented by the TSync_CrcSupport enum */
    uint32_t sequence_id;
    /** @brief flag indicating whether the source port identity field will be considered
     * for CRC calculation as represented by the TSync_CrcSupport enum */
    uint32_t source_port_identity;
} TSync_CrcFlags;

/** @brief This struct holds all the data relevant to the configuration of a timebase resource. */
typedef struct {
    uint8_t fupDataIdList[TSYNC_MAX_FUP_DATA_ID_ENTRIES];
    /** @brief holds the configured mac multicast address for ethernet tsync communication */
    char destMacAddress[TSYNC_MAC_ADDRESS_STRING_LENGTH + 1];
    /** @brief holds the configured immidiate resume time in miliseconds */
    int64_t immediateResumeTimeMs;
    /** @brief holds the configured sync period in miliseconds */
    int64_t syncPeriodMs;
    /** @brief holds the configured follow-up timeout value in miliseconds */
    int64_t followUpTimeoutMs;
    /** @brief holds the configured message compliance as represented by the TSync_MessageCompliance enum */
    uint32_t messageCompliance;
    /** @brief holds the configured vlan priority */
    uint32_t vLanPriority;
    /** @brief holds the number of entries in the fup_data_id_list array. This is either 0 or 16 */
    uint32_t numFupDataIdEntries;
    /** @brief holds the configured CRC support as represented by the TSync_CrcSupport enum */
    uint32_t crcSupport;
    /** @brief holds the configured CRC flags */
    TSync_CrcFlags crcFlags;
    /** @brief holds the configured CRC validation support as represented by the TSync_RxCrcValidation enum */
    uint32_t crcValidation;
    /** @brief holds the configured ptpd role as represented by the TSync_Role enum */
    uint32_t role;
    /** @brief holds the SubTLV configuration */
    TSync_SubTlvConfig subTlvConfig;
    /** @brief holds the sequence counter jump width */
    uint32_t globalTimeSequenceCounterJumpWidth;
} TSync_TimeBaseConfiguration;

#endif  // TSYNC_PTP_LIB_TYPES_H_INCLUDED
