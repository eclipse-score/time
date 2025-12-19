/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_SUBTLV_H
#define SCORE_PTP_SUBTLV_H

#include <string.h>
#include <Crc.h>

#include "score_ptp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Updates the Autosar TLV information into the ptpd buffer.
 * @details
 * This function is used by Provider to write the Autosar TLV information to
 * outgoing buffer.
 * @param[out]  autosarTlvBufferOut    Pointer to buffer where Autosar TLV can
 * be updated.
 * @return None.
 */
void Score_PtpWriteAutosarTlvToBuffer(uint8_t *autosarTlvBufferOut);

/** @brief Reads the Autosar TLV information from the ptpd buffer.
 * @details
 * This function is used by Consumer to read/extract the TLV from the
 * incoming buffer.
 * @param[in]  autosarTlvBufferIn   Pointer to buffer where Autosar Tlv information
 * can be read from.
 * @return TRUE if Autosar SubTLVs were read successfully, FALSE otherwise
 */
Score_PtpBooleanType Score_PtpReadSubTlvFromBuffer(const uint8_t *autosarTlvBufferIn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCORE_PTP_SUBTLV_H */