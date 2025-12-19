/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>
#include "inc/score_ptp_arith.h"
#include "score_ptp_arith_mock.h"

namespace testing {
namespace ptp_daemon_arith_ut {

TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_SameSign_NanoSecBelowLimit_Success){
    score_ptp_arith mArith;
    uint64_t timeValue = 10*SCORE_PTP_NANOSEC;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;
    Score_PtpConvertToTimeStamp(timeValue, 0u,  &timeStampIn1);
    Score_PtpConvertToTimeStamp(timeValue, 0u,  &timeStampIn2);
    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(timeStampIn2.sign, timeStampIn1.sign);
    EXPECT_EQ(resultOut.sign, timeStampIn1.sign);
    EXPECT_EQ(resultOut.sign, timeStampIn2.sign);
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn1.ts.nanoseconds+timeStampIn1.ts.nanoseconds));
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn1)+mArith.getTimeStampSecond(&timeStampIn2)
    );
}

TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_SameSign_NanoSecExceededLimit_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_POSITIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn2.ts.nanoseconds = 2;
    timeStampIn1.ts.seconds = 0u;
    timeStampIn2.ts.seconds = 0u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(timeStampIn2.sign, timeStampIn1.sign);
    EXPECT_EQ(resultOut.sign, timeStampIn1.sign);
    EXPECT_EQ(resultOut.sign, timeStampIn2.sign);
    EXPECT_EQ(resultOut.ts.seconds, 1u);
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn1.ts.nanoseconds+timeStampIn2.ts.nanoseconds-SCORE_PTP_NANOSEC));
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn1)+mArith.getTimeStampSecond(&timeStampIn2)+1u
    );
}

TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_SumIsZero_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 1u;
    timeStampIn2.ts.seconds = 1u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(resultOut.ts.nanoseconds, 0u);
    EXPECT_EQ(mArith.getTimeStampSecond(&resultOut), 0u);
    EXPECT_EQ(resultOut.sign, timeStampIn1.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_DiffSign_SameSec_DifferentNanoSec_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 1u;
    timeStampIn2.ts.seconds = 1u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(mArith.getTimeStampSecond(&resultOut), 0u);
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn2.ts.nanoseconds-timeStampIn1.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn2.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_DiffSign_FirstOpIsGreater_WithGreaterNanoSec_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn1.ts.seconds = 1u;
    timeStampIn2.ts.seconds = 0u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn1)-mArith.getTimeStampSecond(&timeStampIn2)
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn1.ts.nanoseconds-timeStampIn2.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn1.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_DiffSign_FirstOpIsGreater_WithSmallerNanoSec_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 1u;
    timeStampIn2.ts.seconds = 0u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn1)-mArith.getTimeStampSecond(&timeStampIn2)-1u
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, ((timeStampIn1.ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn2.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn1.sign);
}


TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_DiffSign_SecondOpIsGreater_WithGreaterNanoSec_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 0u;
    timeStampIn2.ts.seconds = 1u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn2)-mArith.getTimeStampSecond(&timeStampIn1)
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn2.ts.nanoseconds-timeStampIn1.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn2.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeSumAPI_DifferentSign_SecondOpIsGreater_WithSmallerNanoSec_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn1.ts.seconds = 0u;
    timeStampIn2.ts.seconds = 1u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeSum(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn2)-mArith.getTimeStampSecond(&timeStampIn1)-1u
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, ((timeStampIn2.ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn1.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn2.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeDiffAPI_TimeDiffIsZero_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_POSITIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 0u;
    timeStampIn2.ts.seconds = 0u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeDiff(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(resultOut.ts.nanoseconds, 0u);
    EXPECT_EQ(mArith.getTimeStampSecond(&resultOut), 0u);
    EXPECT_EQ(resultOut.sign, SCORE_PTP_POSITIVE);
}


TEST(Score_PtpArithmetic, PtpCalculateTimeDiffAPI_SecondsValueIsEqual_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_POSITIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 0u;
    timeStampIn2.ts.seconds = 0u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeDiff(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn2.ts.nanoseconds - timeStampIn1.ts.nanoseconds));
    EXPECT_EQ(mArith.getTimeStampSecond(&resultOut), 0u);
    EXPECT_EQ(resultOut.sign, SCORE_PTP_NEGATIVE);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeDiffAPI_FirstOperandIsGreater_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn1.ts.seconds = 1u;
    timeStampIn2.ts.seconds = 0u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeDiff(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn1)-mArith.getTimeStampSecond(&timeStampIn2)
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn1.ts.nanoseconds - timeStampIn2.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn1.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeDiffAPI_FirstOperandIsGreater_WithSmallerNs_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 1u;
    timeStampIn2.ts.seconds = 0u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeDiff(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn1)-mArith.getTimeStampSecond(&timeStampIn2)-1u
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, ((timeStampIn1.ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn2.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn1.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeDiffAPI_SecondOperandIsGreater_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn1.ts.seconds = 0u;
    timeStampIn2.ts.seconds = 1u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeDiff(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn2)-mArith.getTimeStampSecond(&timeStampIn1)
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, (timeStampIn2.ts.nanoseconds - timeStampIn1.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn2.sign);
}

TEST(Score_PtpArithmetic, PtpCalculateTimeDiffAPI_SecondOperandIsGreater_WithSmallerNs_Success){
    score_ptp_arith mArith;
    Score_PtpSignedTimeStampType timeStampIn1;
    Score_PtpSignedTimeStampType timeStampIn2;
    Score_PtpSignedTimeStampType resultOut;

    timeStampIn1.sign = SCORE_PTP_POSITIVE;
    timeStampIn2.sign = SCORE_PTP_NEGATIVE;
    timeStampIn1.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT;
    timeStampIn2.ts.nanoseconds = SCORE_PTP_NS_UPPERLIMIT-100;
    timeStampIn1.ts.seconds = 0u;
    timeStampIn2.ts.seconds = 1u;
    timeStampIn1.ts.secondsHi = 0u;
    timeStampIn2.ts.secondsHi = 0u;

    Score_PtpCalculateTimeDiff(&timeStampIn1, &timeStampIn2, &resultOut);
    EXPECT_EQ(
        mArith.getTimeStampSecond(&resultOut),
        mArith.getTimeStampSecond(&timeStampIn2)-mArith.getTimeStampSecond(&timeStampIn1)-1u
    );
    EXPECT_EQ(resultOut.ts.nanoseconds, ((timeStampIn2.ts.nanoseconds + SCORE_PTP_NANOSEC) - timeStampIn1.ts.nanoseconds));
    EXPECT_EQ(resultOut.sign, timeStampIn2.sign);
}


TEST(Score_PtpArithmetic, PtpConvertToTimeStampAPI_ReturnsPositiveValue_Success){
    score_ptp_arith mArith;
    uint64_t timeValue = 10;
    uint8_t factor = 10;
    Score_PtpSignedTimeStampType timeStampResult;
    Score_PtpConvertToTimeStamp(timeValue, factor, &timeStampResult);
    EXPECT_EQ(timeStampResult.sign, SCORE_PTP_POSITIVE);
}

TEST(Score_PtpArithmetic, PtpInterpolateTimeAPI_ReturnPositiveValue_Success){
    score_ptp_arith mArith;
    TSync_VirtualLocalTimeType vltCurrentIn {SCORE_PTP_NANOSEC, 0u};
    TSync_VirtualLocalTimeType vltPreviousIn {SCORE_PTP_NS_UPPERLIMIT, 0u};
    Score_PtpSignedTimeStampType resultCurrentTimeStampOut;
    resultCurrentTimeStampOut.sign = SCORE_PTP_POSITIVE;
    Score_PtpSignedTimeStampType globalTimeStampPreviousIn;
    uint64_t timeValue = SCORE_PTP_NANOSEC;
    Score_PtpConvertToTimeStamp(timeValue, 0u, &globalTimeStampPreviousIn);
    Score_PtpInterpolateTime(&vltCurrentIn, &vltPreviousIn, &globalTimeStampPreviousIn, &resultCurrentTimeStampOut);
    EXPECT_EQ(resultCurrentTimeStampOut.sign, SCORE_PTP_POSITIVE);
    EXPECT_EQ(resultCurrentTimeStampOut.ts.nanoseconds, 1u);
    EXPECT_EQ(resultCurrentTimeStampOut.ts.seconds, 1u);
    EXPECT_EQ(resultCurrentTimeStampOut.ts.secondsHi, 0u);
}

} // namespace testing
} // namespace ptp_daemon_arith_ut