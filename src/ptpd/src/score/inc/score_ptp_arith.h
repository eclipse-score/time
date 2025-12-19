/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_ARITH_H
#define SCORE_PTP_ARITH_H

#include "score_ptp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Algorithm to add two timestamps.
 * @details
 * This function enables the user to add two timestamps. The timestamps
 * have sign as well. Hence as per the sign, the addition/subtraction shall be
 * performed.
 * @param[in]  timeStampIn1             First Timestamp operand.
 * @param[in]  timeStampIn2             Second Timestamp operand.
 * @param[out] timeStampResultOut       The result is of the operation is
 * updated here.
 * @return None.
 */
void Score_PtpCalculateTimeSum(const Score_PtpSignedTimeStampType *timeStampIn1,
                            const Score_PtpSignedTimeStampType *timeStampIn2,
                            Score_PtpSignedTimeStampType *timeStampResultOut);

/** @brief Algorithm to subtract two timestamps.
 * @details
 * This function enables the user to subtract two timestamps.
 * @param[in]  timeStampIn1             First Timestamp operand.
 * @param[in]  timeStampIn2             Second Timestamp operand.
 * @param[out] timeStampResultOut       The result is of the operation is
 * updated here.
 * @return None.
 */
void Score_PtpCalculateTimeDiff(const Score_PtpSignedTimeStampType *timeStampIn1,
                             const Score_PtpSignedTimeStampType *timeStampIn2,
                             Score_PtpSignedTimeStampType *timeStampResultOut);

/** @brief Algorithm to convert uint64_t(nanoseconds) value to TimeStamp stamp.
 * @details
 * This function enables the user convert uint64_t value into TimeStamp type.
 * It also initializes the sign of the TimeStamp generated as POSITIVE.
 * @param[in]  timeValue           Time value in 64 bits format.
 * @param[in]  factor              Number of bits to shift.
 * @param[out] timeStampResultOut  The result is of the operation is updated
 * here.
 * @return None.
 */
void Score_PtpConvertToTimeStamp(uint64_t timeValue, uint8_t factor, Score_PtpSignedTimeStampType *timeStampResultOut);

/** @brief Algorithm to interpolate the global time based on VirtualLocalTimes.
 * @details
 * This function interpolates the global time based on the passed
 * VirtualLocalTime. The computation is as follows :
 * new_global_time =
 * global_time_passed + (new_virtual_local_time - old_virtual_local_time)
 * @param[in]  vltCurrentIn                The new Virtual Local time.
 * @param[in]  vltPreviousIn               The previous/old Virtual Local Time.
 * @param[in]  globalTimeStampPreviousIn   The previous/old global time.
 * @param[out] resultCurrentTimeStampOut   Pointer to newly computed global
 * time.
 * @return None.
 */
void Score_PtpInterpolateTime(const TSync_VirtualLocalTimeType *vltCurrentIn,
                           const TSync_VirtualLocalTimeType *vltPreviousIn,
                           const Score_PtpSignedTimeStampType *globalTimeStampPreviousIn,
                           Score_PtpSignedTimeStampType *resultCurrentTimeStampOut);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCORE_PTP_ARITH_H */
