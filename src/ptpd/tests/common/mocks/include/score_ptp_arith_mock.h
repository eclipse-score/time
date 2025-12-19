/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "inc/score_ptp_arith.h"
#include <gmock/gmock.h>

class score_ptp_arith {
public:
    uint64_t getTimeStampSecond(Score_PtpSignedTimeStampType* timeStampIn) {
        return (uint64_t)((((uint64_t)(timeStampIn->ts.secondsHi)) << 32) | (timeStampIn->ts.seconds));
    }
};