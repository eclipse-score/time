/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "inc/score_ptp_subtlv.h"
#include "score_ptp_subtlv_mock.h"

using ::testing::_;
using ::testing::Return;

PtpSubTlvMock ptpSubTlvMock;

Score_PtpConsumerDataType consumerData;
Score_PtpProviderDataType providerData;
Score_PtpConfigType ptpConfig = {};

uint8 Crc_CalculateCRC8H2F(const uint8* Crc_DataPtr, uint32 Crc_Length, uint8 Crc_StartValue8,
                           boolean Crc_IsFirstCall) {
    return ptpSubTlvMock.Crc_CalculateCRC8H2F(Crc_DataPtr, Crc_Length, Crc_StartValue8, Crc_IsFirstCall);
}

class ScorePtpSubtlvTest : public ::testing::Test {
   protected:
    PtpSubTlvMock ptp_sub_tlv;
    const uint8 crc_sample_data = 25;

    void SetUp() override {
        ON_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, _, _)).WillByDefault(Return(0));
        // Setup provider
        providerData.followUpMsg.subTlv.userdata.userDataLength = 3u;
        providerData.followUpMsg.subTlv.userdata.userByte0 = 1u;
        providerData.followUpMsg.subTlv.userdata.userByte1 = 2u;
        providerData.followUpMsg.subTlv.userdata.userByte2 = 3u;
        ptpConfig = {};
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(&ptpSubTlvMock);
    }
};

namespace testing {
namespace ptp_daemon_subtlv_ut {

TEST_F(ScorePtpSubtlvTest, Score_PtpWriteAutosarTlvToBuffer_Not_TSYNC_MC_IEEE8021AS_AUTOSAR_Succeed) {
    // Arrange
    uint8_t output[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    // Act
    Score_PtpWriteAutosarTlvToBuffer(&output[0]);
    // Assert
}

TEST_F(ScorePtpSubtlvTest, Score_PtpWriteAutosarTlvToBuffer_No_TSYNC_SUBTLV_SUPPORTED_Succeed) {
    // Arrange
    uint8_t output[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    // Act
    Score_PtpWriteAutosarTlvToBuffer(&output[0]);
    // Assert
}

TEST_F(ScorePtpSubtlvTest, Score_PtpWriteAutosarTlvToBuffer_InputIsNull_Failed) {
    // Arrange
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    // Act
    Score_PtpWriteAutosarTlvToBuffer(nullptr);
    // Assert
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpWriteAutosarTlvToBuffer_NoCRCFlagAreSet_Succeed) {
    // Arrange
    uint8_t output[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    // Set bit
    ptpConfig.configGlobals.crcTimeFlags = 0;
    // Check bit
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).WillRepeatedly(Return(crc_sample_data));
    // Act
    Score_PtpWriteAutosarTlvToBuffer(&output[0]);
    // Assert
    Score_PtpAutosarTlvType autosarTlv;
    memcpy((void*)(&autosarTlv), (void*)(&output), sizeof(Score_PtpAutosarTlvType));
    EXPECT_GE(autosarTlv.tlvType, 0u);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.subtlvType, SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.subtlvLength, SCORE_PTP_FUP_SUBTLV_LEN_FIELD_TIMESECURED);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.crcTimeFlags, ptpConfig.configGlobals.crcTimeFlags);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataLen, providerData.followUpMsg.subTlv.userdata.userDataLength);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte0, providerData.followUpMsg.subTlv.userdata.userByte0);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte1, providerData.followUpMsg.subTlv.userdata.userByte1);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte2, providerData.followUpMsg.subTlv.userdata.userByte2);
    EXPECT_EQ(autosarTlv.subTLVUserdata.subtlvType, SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC);
    EXPECT_EQ(autosarTlv.subTLVUserdata.subtlvLength, SCORE_PTP_FUP_SUBTLV_LEN_FIELD_USERDATA);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpWriteAutosarTlvToBuffer_AllCRCFlagAreSet_Succeed) {
    // Arrange
    uint8_t output[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    // Set bit
    ptpConfig.configGlobals.crcTimeFlags = 0;
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK);
    // Check bit
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).WillRepeatedly(Return(crc_sample_data));
    // Act
    Score_PtpWriteAutosarTlvToBuffer(&output[0]);
    // Assert
    Score_PtpAutosarTlvType autosarTlv;
    memcpy((void*)(&autosarTlv), (void*)(&output), sizeof(Score_PtpAutosarTlvType));
    EXPECT_GE(autosarTlv.tlvType, 0u);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.subtlvType, SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.subtlvLength, SCORE_PTP_FUP_SUBTLV_LEN_FIELD_TIMESECURED);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.crcTime0, crc_sample_data);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.crcTime1, crc_sample_data);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.crcTimeFlags, ptpConfig.configGlobals.crcTimeFlags);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataLen, providerData.followUpMsg.subTlv.userdata.userDataLength);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte0, providerData.followUpMsg.subTlv.userdata.userByte0);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte1, providerData.followUpMsg.subTlv.userdata.userByte1);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte2, providerData.followUpMsg.subTlv.userdata.userByte2);
    EXPECT_EQ(autosarTlv.subTLVUserdata.crcUserdata, crc_sample_data);
    EXPECT_EQ(autosarTlv.subTLVUserdata.subtlvType, SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC);
    EXPECT_EQ(autosarTlv.subTLVUserdata.subtlvLength, SCORE_PTP_FUP_SUBTLV_LEN_FIELD_USERDATA);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpWriteAutosarTlvToBuffer_WithoutCRCSupported_Succeed) {
    // Arrange
    uint8_t output[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).Times(0);
    // Act
    Score_PtpWriteAutosarTlvToBuffer(&output[0]);
    // Assert
    Score_PtpSubtlvUserdataType userData;
    uint8_t* userPtr = output + SCORE_PTP_FUP_AUTOSAR_TLV_INFO_LENGTH + SCORE_PTP_FUP_SUBTLV_STATUS_LENGTH;
    memcpy((void*)(&userData), (void*)(userPtr), sizeof(Score_PtpSubtlvUserdataType));
    EXPECT_EQ(userData.userdataLen, providerData.followUpMsg.subTlv.userdata.userDataLength);
    EXPECT_EQ(userData.userdataByte0, providerData.followUpMsg.subTlv.userdata.userByte0);
    EXPECT_EQ(userData.userdataByte1, providerData.followUpMsg.subTlv.userdata.userByte1);
    EXPECT_EQ(userData.userdataByte2, providerData.followUpMsg.subTlv.userdata.userByte2);
    EXPECT_EQ(userData.crcUserdata, 0u);
    EXPECT_EQ(userData.subtlvType, SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC);
    EXPECT_EQ(userData.subtlvLength, SCORE_PTP_FUP_SUBTLV_LEN_FIELD_USERDATA);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpWriteAutosarTlvToBuffer_With_numFupDataIdEntries_Succeed) {
    // Arrange
    uint8_t output[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.numFupDataIdEntries = 1u;
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).WillRepeatedly(Return(crc_sample_data));
    // Act
    Score_PtpWriteAutosarTlvToBuffer(&output[0]);
    // Assert
    Score_PtpAutosarTlvType autosarTlv;
    memcpy((void*)(&autosarTlv), (void*)(&output), sizeof(Score_PtpAutosarTlvType));
    EXPECT_GE(autosarTlv.tlvType, 0u);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.subtlvType, SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.subtlvLength, SCORE_PTP_FUP_SUBTLV_LEN_FIELD_TIMESECURED);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.crcTime0, crc_sample_data);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.crcTime1, crc_sample_data);
    EXPECT_EQ(autosarTlv.subTLVTimeSecured.crcTimeFlags, ptpConfig.configGlobals.crcTimeFlags);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataLen, providerData.followUpMsg.subTlv.userdata.userDataLength);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte0, providerData.followUpMsg.subTlv.userdata.userByte0);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte1, providerData.followUpMsg.subTlv.userdata.userByte1);
    EXPECT_EQ(autosarTlv.subTLVUserdata.userdataByte2, providerData.followUpMsg.subTlv.userdata.userByte2);
    EXPECT_EQ(autosarTlv.subTLVUserdata.crcUserdata, crc_sample_data);
    EXPECT_EQ(autosarTlv.subTLVUserdata.subtlvType, SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC);
    EXPECT_EQ(autosarTlv.subTLVUserdata.subtlvLength, SCORE_PTP_FUP_SUBTLV_LEN_FIELD_USERDATA);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTlvFromBuffer_NoValueSetUp_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTlvFromBuffer_InputIsNull_Succeed) {
    // Arrange
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    auto res = Score_PtpReadSubTlvFromBuffer(nullptr);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTlvFromBuffer_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set bit
    ptpConfig.configGlobals.crcTimeFlags = 0;
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK);
    SCORE_PTP_SET_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK);
    // Check bit
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK));
    EXPECT_TRUE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).WillRepeatedly(Return(crc_sample_data));
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    autosarTlv.subTLVTimeSecured.crcTime0 = crc_sample_data;
    autosarTlv.subTLVTimeSecured.crcTime1 = crc_sample_data;
    autosarTlv.subTLVUserdata.crcUserdata = crc_sample_data;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userDataLength, autosarTlv.subTLVUserdata.userdataLen);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte0, autosarTlv.subTLVUserdata.userdataByte0);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte1, autosarTlv.subTLVUserdata.userdataByte1);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte2, autosarTlv.subTLVUserdata.userdataByte2);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTlvFromBuffer_WithNoCrcFlagSet_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set bit
    ptpConfig.configGlobals.crcTimeFlags = 0;
    // Check bit
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_MESSAGE_LENGTH_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_DOMAIN_NUMBER_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_CORRECTION_FIELD_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SOURCE_PORT_ID_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_SEQUENCE_ID_MASK));
    EXPECT_FALSE(SCORE_PTP_CHECK_CRC_FLAG(ptpConfig.configGlobals.crcTimeFlags, SCORE_PTP_CRC_TIMEFLAG_PRECISE_ORIGIN_TIME_MASK));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).WillRepeatedly(Return(crc_sample_data));
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    autosarTlv.subTLVTimeSecured.crcTime0 = crc_sample_data;
    autosarTlv.subTLVTimeSecured.crcTime1 = crc_sample_data;
    autosarTlv.subTLVUserdata.crcUserdata = crc_sample_data;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userDataLength, autosarTlv.subTLVUserdata.userdataLen);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte0, autosarTlv.subTLVUserdata.userdataByte0);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte1, autosarTlv.subTLVUserdata.userdataByte1);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte2, autosarTlv.subTLVUserdata.userdataByte2);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTlvFromBuffer_WithZeroSubTlvConfigured_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.configGlobals.configSubTlvLength = 0u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVUserdata_With_CRC_And_TSYNC_CRC_NOT_VALIDATED_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_NOT_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVUserdata_With_TSYNC_CRC_IGNORED_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_IGNORED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userDataLength, autosarTlv.subTLVUserdata.userdataLen);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte0, autosarTlv.subTLVUserdata.userdataByte0);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte1, autosarTlv.subTLVUserdata.userdataByte1);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte2, autosarTlv.subTLVUserdata.userdataByte2);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVUserdata_With_SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVUserdata_With_SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_NOT_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITHOUT_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userDataLength, autosarTlv.subTLVUserdata.userdataLen);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte0, autosarTlv.subTLVUserdata.userdataByte0);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte1, autosarTlv.subTLVUserdata.userdataByte1);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte2, autosarTlv.subTLVUserdata.userdataByte2);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTLVUserdata_With_Invalid_SubtlvType_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_NOT_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITHOUT_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTLVUserdata_With_Invalid_CrcValidation_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = 0x10;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITHOUT_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVUserdata_With_numFupDataIdEntries_GreaterThanZero_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.timebaseConfig.numFupDataIdEntries = 1u;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    consumerData.followUpMsg.followupHeaderInfo.sequenceId = 0x1;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITHOUT_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    autosarTlv.subTLVUserdata.crcUserdata = crc_sample_data;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).WillRepeatedly(Return(crc_sample_data));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userDataLength, autosarTlv.subTLVUserdata.userdataLen);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte0, autosarTlv.subTLVUserdata.userdataByte0);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte1, autosarTlv.subTLVUserdata.userdataByte1);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte2, autosarTlv.subTLVUserdata.userdataByte2);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVUserdata_With_crcComputed_Eq_crcUserData_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    autosarTlv.subTLVUserdata.crcUserdata = 10;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE))
        .WillOnce(Return(static_cast<uint8>(10)));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userDataLength, autosarTlv.subTLVUserdata.userdataLen);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte0, autosarTlv.subTLVUserdata.userdataByte0);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte1, autosarTlv.subTLVUserdata.userdataByte1);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte2, autosarTlv.subTLVUserdata.userdataByte2);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVUserdata_With_InconsistenciesCRC_Failed) {
    // Arrange
    /**/
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    autosarTlv.subTLVUserdata.crcUserdata = 123;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE))
        .WillOnce(Return(static_cast<uint8>(10)));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTLVTimeSecured_WithoutSupportFor_followUpTimeSubTLV_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    // Set up input
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.subTLVStatus.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.subTLVUserdata.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITH_CRC;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVTimeSecured_CRCIsNotValid_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_NOT_VALIDATED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTLVTimeSecured_Without_SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_USERDATA_WITHOUT_CRC;
    autosarTlv.tlvLength = 1;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTLVTimeSecured_SubTlvReceiveDoNotMatchConfiguredSubTlv_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.tlvLength = 0;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.configGlobals.configSubTlvLength = 123;
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTLVStatus_With_unsupported_SubTLV_ReturnZero_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    ptpConfig.configGlobals.followupLength = 8;
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_STATUS_WITH_CRC;
    autosarTlv.tlvLength = 1u;
    autosarTlv.subTLVTimeSecured.crcTime0 = crc_sample_data;
    autosarTlv.subTLVTimeSecured.crcTime1 = crc_sample_data;
    autosarTlv.subTLVUserdata.crcUserdata = crc_sample_data;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE)).WillRepeatedly(Return(crc_sample_data));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userDataLength, autosarTlv.subTLVUserdata.userdataLen);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte0, autosarTlv.subTLVUserdata.userdataByte0);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte1, autosarTlv.subTLVUserdata.userdataByte1);
    EXPECT_EQ(consumerData.followUpMsg.subTlv.userdata.userByte2, autosarTlv.subTLVUserdata.userdataByte2);
}

TEST_F(ScorePtpSubtlvTest, DISABLED_Score_PtpReadSubTLVTimeSecured_With_TimeSecuredWithInconsistenciesCRC_Failed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.tlvLength = 1;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    EXPECT_CALL(ptpSubTlvMock, Crc_CalculateCRC8H2F(_, _, 0u, SCORE_PTP_TRUE))
        .WillRepeatedly(Return(static_cast<uint8>(12)));
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_FALSE);
}

TEST_F(ScorePtpSubtlvTest, Score_PtpReadSubTLVTimeSecured_Without_followUpTimeSubTLV_TSYNC_SUBTLV_SUPPORTED_Succeed) {
    // Arrange
    uint8_t input[SCORE_PTP_TOTAL_AUTOSAR_TLV_LENGTH];
    Score_PtpAutosarTlvType autosarTlv;
    autosarTlv.subTLVTimeSecured.subtlvType = SCORE_PTP_FUP_SUBTLV_TYPE_TIMESECURED;
    autosarTlv.tlvLength = 1;
    memcpy((void*)(&input), (void*)(&autosarTlv), sizeof(Score_PtpAutosarTlvType));
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.configGlobals.configSubTlvLength = 1u;
    // Act
    auto res = Score_PtpReadSubTlvFromBuffer(&input[0]);
    // Assert
    EXPECT_EQ(res, SCORE_PTP_TRUE);
}

}  // namespace ptp_daemon_subtlv_ut
}  // namespace testing
