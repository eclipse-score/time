/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_PTP_SUBTLV_MOCK_H
#define SCORE_PTP_SUBTLV_MOCK_H

#include "inc/score_ptp_subtlv.h"
#include <gmock/gmock.h>

class PtpSubTlvMock {
public:
    MOCK_METHOD(uint8, Crc_CalculateCRC8H2F, (const uint8*, uint32, uint8, boolean));
};

#endif /* SCORE_PTP_SUBTLV_MOCK_H */
