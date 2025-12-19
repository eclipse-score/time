/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_CONFIG_H
#define SCORE_PTP_CONFIG_H

#include "score_ptp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Type to store all the global variables that are dependent on configuration.
 * The configuration is read from the tsync-ptp-lib. Depending on the configuration, each of the items below will be
 * intialized once during intialization and used during the lifetime.
 */
typedef struct {
    /** @brief The variable holding the length of the follow-up message.
     * The length depends on the configuration parameter "MessageCompliance" */
    uint8_t followupLength;

    /** @brief The variable holding the length of all the SubTLVs configured */
    uint8_t configSubTlvLength;

    /** @brief The variable holds the bit masks for the configured time flags */
    uint8_t crcTimeFlags;
} Score_ConfigGlobals;

/** @brief Type for storing the configuration information and information that
 * is derived from the configuration. Configuration info mainly comes from the
 * command line.
 */
typedef struct {
    /** @brief TimeBase id associated with the current ptpd instance */
    uint16_t timebaseId;

    /** @brief The static GlobalPropogationDelay - this is statically configured
     * as per autosar requirement. */
    /* Todo: This Pdelay value must be included in TSync_TimeBaseConfiguration
    going forward - Perhaps when we support AUTOSAR 23-11 */
    Score_PtpSignedTimeStampType pdelay;

    /**  @brief Time base handle assciated with the above TimeBase id */
    TSync_TimeBaseHandleType timebaseHandle;

    /** @brief This structure holds all the static configuration for the
     * timebase */
    TSync_TimeBaseConfiguration timebaseConfig;

    Score_ConfigGlobals configGlobals;
} Score_PtpConfigType;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCORE_PTP_CONFIG_H */
