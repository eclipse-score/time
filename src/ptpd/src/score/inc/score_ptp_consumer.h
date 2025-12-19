/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_CONSUMER_H
#define SCORE_PTP_CONSUMER_H

#include <stdint.h> 
#include "score_ptp_types.h"
#include "score_ptp_arith.h"
#include "score_ptp_subtlv.h"
#include "score_ptp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The structure that is used to store the information for computation
 * of globalTime */
extern Score_PtpConsumerDataType consumerData;

/** @brief Function to update the follow-up information received .
 * @details
 * This function stores the follow-up information into a global data structure.
 * The information is originally received from the follow-up message.
 * @param[in]  originTimeIn          Precise Origin Time.
 * @param[in]  corrTimeIn            Correction field.
 * @param[in]  fupInfoIn             Pointer to buffer containing fup info.
 * @return None.
 */
void Score_PtpReadFupInfoFromBuffer(const uint8_t *originTimeIn, 
                                  const uint8_t *corrTimeIn, 
                                  const uint8_t *fupInfoIn);

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
void Score_PtpReadIngressVlt(Score_PtpIngressVltType vltIngress);

/** @brief Computes the GlobalTime and calls BusSetGlobalTime.
 * @details
 * This functions performs the globalTime computation and makes call to
 * BusSetGlobalTime. The fomula for globalTime computation is:
 *    globalTime = PreciseOriginTime + correctionfield + (T2vlt-T1vlt) + Pdelay;
 *    localTime = T2vlt,
 *    measureData = Pdelay
 * @param None.
 * @return None.
 */
void Score_PtpBusSetGlobalTime(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCORE_PTP_CONSUMER_H */
