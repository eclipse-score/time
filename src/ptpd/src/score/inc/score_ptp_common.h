/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_COMMON_H
#define SCORE_PTP_COMMON_H

#include <errno.h>
#include <pthread.h>
#include "score_ptp_config.h"
#include "score_ptp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Global configuration variable for PTP */
extern Score_PtpConfigType ptpConfig;

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
Score_PtpReturnType Score_PtpInitialize(double pdelay, uint16_t domainNumber, Score_PtpBooleanType isProvider);

/** @brief Deinit function to perform graceful shutdown.
 * @details
 * This functions performs all the activities associated with graceful shutdown
 * - Like close the connection with TSync, set the tsync handle to NULL.
 * @param None.
 * @return None.
 */
void Score_PtpDeinitialize(void);

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
void Score_PtpReadEgressVlt(Score_PtpEgressVltType vltEgress);

/** @brief Update the GlobalTime and VirtualLocalTime for T0 and T0Vlt
 * respectively.
 * @details
 * This function gets the Virtual local Time and GlobalTime from TSync and
 * store it as T0 and T0Vlt. These values are used later to compute OriginTime.
 * @param None.
 * @return None.
 */
void Score_PtpBusGetGlobalTime(void);

/** @brief Fetch T4Vlt and compute PreciseOriginTime .
 * @details
 * This functions fetches T4Vlt from tsync. Then based on T4Vlt and T0Vlt
 * (which is stored previously in Provider information), T0
 * is interpolated to compute the PreciseOriginTime.
 * @param None.
 * @return None.
 */
void Score_PtpProviderComputeOriginTime(void);

/** @brief Fetch PreciseOriginTime from Provider data-structure.
 * @details
 * This functions fetches origin time that was computed and stored in provider
 * data-structure based on T4Vlt and T0Vlt
 * @param originTimeOut      Pointer to store the computed origin time.
 * @return None.
 */
void Score_PtpWriteOriginTimeToBuffer(uint8_t *originTimeOut, uint8_t originTimeSize);

/** @brief Callback function to indicate Immediate time transmission.
 * @details
 * This function is called by TSync to indicate that immediate time transmission
 * is needed. It sets immediateTimeTransmission variable to TRUE. This will enable the time
 * transmission on the Bus in the next cycle.
 * @param[in]  domainId        The domain id for which time has to be
 * transmitted.
 * @return E_OK if successful, E_NOT_OK otherwise.
 */
TSync_ReturnType Score_PtpTransmitGlobalTimeCallback(TSync_SynchronizedTimeBaseType domainId);

/** @brief Returns if Immediate time transmission is set to TRUE.
 * @details
 * This function returns if the Immediate time transmission is set to TRUE.
 * @param[in]  None
 * @return TRUE if Immediate time transmission is TRUE, FALSE otherwise.
 */
Score_PtpBooleanType Score_PtpIsImmediateTimeTriggerTrue(void);

/** @brief Resets the Immediate time transmission flag to FALSE.
 * @details
 * This function resets the Immediate time transmission flag to FALSE. It shall
 * be called as soon as the transmission is initiated.
 * @param[in]  None.
 * @return None.
 */
void Score_PtpResetImmediateTimeTrigger(void);

/** @brief Configure function to read configuration from tsync-ptp-lib.
 * @details
 * This function retrieves the configuration data from tsync-ptp-lib during
 * the startup. The configuation is stored into the global configuration
 * structure.
 * @param[in]  isProvider    If the ptpd instance is Provider.
 * @return Score_PtpReturnType  SCORE_PTP_E_OK for successful config update
 *                             SCORE_PTP_E_NOT_OK otherwise.
 */
Score_PtpReturnType Score_PtpConfigure(Score_PtpBooleanType isProvider);

/** @brief Stores the followup header information.
 * @details
 * This function reads the followup header information from the ptpd data structures(open source data structure)
 * into the global structure used by SCORE extensions.The values in this header files are as received by the protocol
 * expect that they are converted from big endian to host format.
 * @return None.
 */
struct MsgHeaderTag;
void Score_PtpExtractFupHeaderInfo(const struct MsgHeaderTag *followupHeaderInfoIn, Score_PtpBooleanType isProvider);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCORE_PTP_COMMON_H */
