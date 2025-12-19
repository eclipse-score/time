/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "../inc/score_ptp_arith.h"

#include <stdlib.h>
#include <string.h>

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
                            Score_PtpSignedTimeStampType *timeStampResultOut) {
    /* local variable declaration */
    uint64_t timeStamp1;
    uint64_t timeStamp2;
    uint64_t timeStampSum;

    /* Convert the TimeStamps(seconds part) to uint64_t */
    timeStamp1 =
        (uint64_t)((((uint64_t)(timeStampIn1->ts.secondsHi)) << 32u) | (timeStampIn1->ts.seconds));
    timeStamp2 =
        (uint64_t)((((uint64_t)(timeStampIn2->ts.secondsHi)) << 32u) | (timeStampIn2->ts.seconds));
    timeStampSum = 0u;

    /* Check signs of both Timestamps are same */
    if (timeStampIn1->sign == timeStampIn2->sign) {
        /* Add nanoseconds part */
        timeStampResultOut->ts.nanoseconds = timeStampIn1->ts.nanoseconds + timeStampIn2->ts.nanoseconds;

        /* Add seconds part now*/
        timeStampSum = timeStamp1 + timeStamp2;

        /* If nanoseconds is greater than 999999999(0x3B9AC9FFU), max
        nanoseconds are reached. Time to increment the seconds count by 1 */
        if (SCORE_PTP_NS_UPPERLIMIT < timeStampResultOut->ts.nanoseconds) {
            timeStampSum += 1u;
            timeStampResultOut->ts.nanoseconds -= SCORE_PTP_NANOSEC;
        }
        /* Take over the sign from either of the TimeStamps - as both times are
        same */
        timeStampResultOut->sign = timeStampIn1->sign;
    }
    /* Since the signs are different, subtraction to be carried out */
    else {
        /* The seconds part of both the Timestamps are same */
        if (timeStamp1 == timeStamp2) {
            timeStampSum = 0u;

            /* Nanoseconds of first TimeStamp is greater than the seconds */
            if (timeStampIn1->ts.nanoseconds >= timeStampIn2->ts.nanoseconds) {
                timeStampResultOut->ts.nanoseconds = timeStampIn1->ts.nanoseconds - timeStampIn2->ts.nanoseconds;
                timeStampResultOut->sign = timeStampIn1->sign;
            } else {
                timeStampResultOut->ts.nanoseconds = timeStampIn2->ts.nanoseconds - timeStampIn1->ts.nanoseconds;
                timeStampResultOut->sign = timeStampIn2->sign;
            }
        }
        /* The seconds part of first TimeStamp is greater than second */
        else if (timeStamp1 > timeStamp2) {
            /* Nanoseconds of first TimeStamp is greater than the seconds */
            if (timeStampIn1->ts.nanoseconds >= timeStampIn2->ts.nanoseconds) {
                timeStampResultOut->ts.nanoseconds = timeStampIn1->ts.nanoseconds - timeStampIn2->ts.nanoseconds;
                timeStampSum = timeStamp1 - timeStamp2;
                timeStampResultOut->sign = timeStampIn1->sign;
            } else {
                timeStampResultOut->ts.nanoseconds =
                    (timeStampIn1->ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn2->ts.nanoseconds;
                timeStampSum = (timeStamp1 - 1u) - timeStamp2;
                timeStampResultOut->sign = timeStampIn1->sign;
            }
        } else {
            if (timeStampIn1->ts.nanoseconds > timeStampIn2->ts.nanoseconds) {
                timeStampResultOut->ts.nanoseconds =
                    (timeStampIn2->ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn1->ts.nanoseconds;
                timeStampSum = (timeStamp2 - 1u) - timeStamp1;
                timeStampResultOut->sign = timeStampIn2->sign;
            } else {
                timeStampResultOut->ts.nanoseconds = timeStampIn2->ts.nanoseconds - timeStampIn1->ts.nanoseconds;
                timeStampSum = timeStamp2 - timeStamp1;
                timeStampResultOut->sign = timeStampIn2->sign;
            }
        }
    }
    /* Updating seconds and secondsHi of timeStampResultOut from local variable */
    timeStampResultOut->ts.secondsHi =
        (uint16_t)(((timeStampSum) & (SCORE_PTP_HIGHER_DOUBLE_WORD_MASK)) >> 32u);
    timeStampResultOut->ts.seconds = (uint32_t)((timeStampSum) & (SCORE_PTP_LOWER_DOUBLE_WORD_MASK));
} /* End of function Score_PtpCalculateTimeSum */

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
                             Score_PtpSignedTimeStampType *timeStampResultOut) {
    /* local variable declaration */
    uint64_t timeStamp1;
    uint64_t timeStamp2;
    uint64_t timeStampDiff;

    /* Convert the TimeStamps(seconds part) to uint64_t */
    timeStamp1 = (uint64_t)((((uint64_t)(timeStampIn1->ts.secondsHi)) << 32) | (timeStampIn1->ts.seconds));
    timeStamp2 = (uint64_t)((((uint64_t)(timeStampIn2->ts.secondsHi)) << 32) | (timeStampIn2->ts.seconds));

    timeStampDiff = 0u;

    /* The seconds part are equal */
    if (timeStamp1 == timeStamp2) {
        timeStampDiff = 0u;

        /* First operand is greater, hence carry out the subtraction and take
        over the POSITIVE sign */
        if (timeStampIn1->ts.nanoseconds >= timeStampIn2->ts.nanoseconds) {
            timeStampResultOut->ts.nanoseconds = timeStampIn1->ts.nanoseconds - timeStampIn2->ts.nanoseconds;
            timeStampResultOut->sign = SCORE_PTP_POSITIVE;
        }
        /* Since the second operand is greater, take over the Negative sign */
        else {
            timeStampResultOut->ts.nanoseconds = timeStampIn2->ts.nanoseconds - timeStampIn1->ts.nanoseconds;
            timeStampResultOut->sign = SCORE_PTP_NEGATIVE;
        }
    }
    /* First operand is greater */
    else if (timeStamp1 > timeStamp2) {
        /* The sign in both the cases below would be POSITIVE as the first
        operand is greater */
        /* When nanoseconds of first operand is greater than second, simple
        subtraction can be carried */
        if (timeStampIn1->ts.nanoseconds >= timeStampIn2->ts.nanoseconds) {
            timeStampResultOut->ts.nanoseconds = timeStampIn1->ts.nanoseconds - timeStampIn2->ts.nanoseconds;
            timeStampDiff = timeStamp1 - timeStamp2;
            timeStampResultOut->sign = SCORE_PTP_POSITIVE;
        }
        /* When nanoseconds of first operand is smaller than second(however the
        seconds part of the first operand is greater ), a borrow operation is
        necessary */
        else {
            timeStampResultOut->ts.nanoseconds =
                (timeStampIn1->ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn2->ts.nanoseconds;
            timeStampDiff = (timeStamp1 - 1u) - timeStamp2;
            timeStampResultOut->sign = SCORE_PTP_POSITIVE;
        }
    }
    /* Second operand is greater */
    else {
        /* The sign in both the cases below would be NEGATIVE as the second
        operand is greater than first */
        if (timeStampIn1->ts.nanoseconds > timeStampIn2->ts.nanoseconds) {
            timeStampResultOut->ts.nanoseconds =
                (timeStampIn2->ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn1->ts.nanoseconds;
            timeStampDiff = (timeStamp2 - 1u) - timeStamp1;
            timeStampResultOut->sign = SCORE_PTP_NEGATIVE;
        } else {
            timeStampResultOut->ts.nanoseconds = timeStampIn2->ts.nanoseconds - timeStampIn1->ts.nanoseconds;
            timeStampDiff = timeStamp2 - timeStamp1;
            timeStampResultOut->sign = SCORE_PTP_NEGATIVE;
        }
    }
    /* Update seconds and secondsHi of timeStampResultOut from local variable */
    timeStampResultOut->ts.secondsHi =
        (uint16_t)(((timeStampDiff) & (SCORE_PTP_HIGHER_DOUBLE_WORD_MASK)) >> 32u);
    timeStampResultOut->ts.seconds = (uint32_t)((timeStampDiff) & (SCORE_PTP_LOWER_DOUBLE_WORD_MASK));

} /* End of function Score_PtpCalculateTimeDiff */

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
void Score_PtpConvertToTimeStamp(uint64_t timeValue, uint8_t factor, Score_PtpSignedTimeStampType *timeStampResultOut) {
    /* Local variable declaration */
    uint64_t timeValueTemp;

    /* Convert from Bitfield to Time */
    timeValueTemp = timeValue >> factor;

    /* Obtain nanoseconds part of time */
    timeStampResultOut->ts.nanoseconds = (uint32_t)(timeValueTemp % SCORE_PTP_NANOSEC);

    /* Obtain Seconds part of time */
    timeValueTemp = timeValueTemp / SCORE_PTP_NANOSEC;

    /* Initialising seconds and secondsHi of Time */
    timeStampResultOut->ts.secondsHi =
        (uint16_t)(((timeValueTemp) & (SCORE_PTP_HIGHER_DOUBLE_WORD_MASK)) >> 32u);
    timeStampResultOut->ts.seconds = (uint32_t)((timeValueTemp) & (SCORE_PTP_LOWER_DOUBLE_WORD_MASK));

    /* Initializes the sign */
    timeStampResultOut->sign = SCORE_PTP_POSITIVE;

} /* End of function Score_PtpConvertToTimeStamp */

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
                           Score_PtpSignedTimeStampType *resultCurrentTimeStampOut) {
    Score_PtpSignedTimeStampType vltCurrentTimeStamp;
    Score_PtpSignedTimeStampType vltPreviousTimeStamp;
    Score_PtpSignedTimeStampType vltDiff;
    Score_PtpSignedTimeStampType globalTimeResult;
    uint64_t vltCurrentTimeValue, vltPreviousTimeValue = 0u;

    memset((void *)(&vltCurrentTimeStamp.ts), 0u, sizeof(TSync_TimeStampType));
    vltCurrentTimeStamp.sign = SCORE_PTP_POSITIVE;

    memset((void *)(&vltPreviousTimeStamp.ts), 0u, sizeof(TSync_TimeStampType));
    vltPreviousTimeStamp.sign = SCORE_PTP_POSITIVE;

    memset((void *)(&vltDiff.ts), 0u, sizeof(TSync_TimeStampType));
    vltDiff.sign = SCORE_PTP_POSITIVE;

    memset((void *)(&globalTimeResult.ts), 0u, sizeof(TSync_TimeStampType));
    globalTimeResult.sign = SCORE_PTP_POSITIVE;

    vltCurrentTimeValue = ((uint64_t)vltCurrentIn->nanosecondsHi << 32) | vltCurrentIn->nanosecondsLo;
    vltPreviousTimeValue = ((uint64_t)vltPreviousIn->nanosecondsHi << 32) | vltPreviousIn->nanosecondsLo;

    /* Convert and store the current time */
    Score_PtpConvertToTimeStamp(vltCurrentTimeValue, 0u, &vltCurrentTimeStamp);

    /* Convert and store the current time */
    Score_PtpConvertToTimeStamp(vltPreviousTimeValue, 0u, &vltPreviousTimeStamp);

    Score_PtpCalculateTimeDiff(&vltCurrentTimeStamp, &vltPreviousTimeStamp, &vltDiff);

    Score_PtpCalculateTimeSum(globalTimeStampPreviousIn, &vltDiff, &globalTimeResult);

    resultCurrentTimeStampOut->ts.secondsHi = globalTimeResult.ts.secondsHi;
    resultCurrentTimeStampOut->ts.seconds = globalTimeResult.ts.seconds;
    resultCurrentTimeStampOut->ts.nanoseconds = globalTimeResult.ts.nanoseconds;
}
