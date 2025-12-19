/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include <algorithm>

#include "inc/score_ptp_common.h"
#include "score_ptp_common_mock.h"
#include "ptp_datatypes.h"

using ::testing::_;
using ::testing::Return;

// Define global function
PtpCommonMock ptpCommonMock;

Score_PtpConsumerDataType consumerData;
Score_PtpProviderDataType providerData;
class ScorePtpCommonTest : public ::testing::Test {
   protected:
    PtpCommonMock ptp_common;
    const uint16_t testDomainNumber = 1;
    // Because all the function that interact with TSync_TimeBaseHandleType is mocked
    // For simplicity Timebase_handler_ is just an address to a int variable
    const int Timebase_handler_ = 0;

    void SetUp() override {
        ON_CALL(ptpCommonMock, TSync_Open()).WillByDefault(Return(E_OK));
        ON_CALL(ptpCommonMock, TSync_OpenTimebase(_)).WillByDefault(Return((void*)(&Timebase_handler_)));
        ON_CALL(ptpCommonMock, TSync_RegisterTransmitGlobalTimeCallback(_, _)).WillByDefault(Return(E_OK));
        ON_CALL(ptpCommonMock, Score_PtpTransmitGlobalTimeCallback(_)).WillByDefault(Return(E_OK));
        ON_CALL(ptpCommonMock, TSync_GetTimebaseConfiguration(_, _, _)).WillByDefault(Return(E_OK));
        ptpConfig = {};
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(&ptpCommonMock);
    }
};

namespace testing {
namespace ptp_daemon_common_ut {

TEST_F(ScorePtpCommonTest, Score_PtpInitialize_WithTsyncOpenFailed_Failed) {
    // Arrange
    EXPECT_CALL(ptpCommonMock, TSync_Open()).WillOnce(Return(E_NOT_OK));
    double pdelay = 0;
    Score_PtpBooleanType isProvider = false;
    // Act
    Score_PtpReturnType result = Score_PtpInitialize(pdelay, testDomainNumber, isProvider);
    // Assert
    EXPECT_EQ(result, SCORE_PTP_E_NOT_OK);
    EXPECT_EQ(ptpConfig.timebaseHandle, nullptr);
}

TEST_F(ScorePtpCommonTest, Score_PtpInitialize_WithTsyncOpenTimebaseFailed_Failed) {
    // Arrange
    EXPECT_CALL(ptpCommonMock, TSync_OpenTimebase((TSync_SynchronizedTimeBaseType)(testDomainNumber)))
        .WillOnce(Return(nullptr));
    double pdelay = 0;
    Score_PtpBooleanType isProvider = false;
    // Act
    Score_PtpReturnType result = Score_PtpInitialize(pdelay, testDomainNumber, isProvider);
    // Assert
    EXPECT_EQ(result, SCORE_PTP_E_NOT_OK);
    EXPECT_EQ(ptpConfig.timebaseHandle, nullptr);
}

TEST_F(ScorePtpCommonTest, Score_PtpInitialize_WithRegisterTransmitGlobalTimeCallbackFailed_Failed) {
    // Arrange
    EXPECT_CALL(ptpCommonMock, TSync_RegisterTransmitGlobalTimeCallback(_, _)).WillOnce(Return(E_NOT_OK));
    double pdelay = 0;
    Score_PtpBooleanType isProvider = true;
    // Act
    Score_PtpReturnType result = Score_PtpInitialize(pdelay, testDomainNumber, isProvider);
    // Assert
    EXPECT_EQ(result, SCORE_PTP_E_NOT_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpInitialize_WithPtpConfigureFailed_Failed) {
    // Arrange
    EXPECT_CALL(ptpCommonMock, TSync_GetTimebaseConfiguration(_, _, _)).WillOnce(Return(E_NOT_OK));
    double pdelay = 0;
    Score_PtpBooleanType isProvider = true;
    // Act
    Score_PtpReturnType result = Score_PtpInitialize(pdelay, testDomainNumber, isProvider);
    // Assert
    EXPECT_EQ(result, SCORE_PTP_E_NOT_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpInitialize_WithisProviderTrue_Success) {
    // Arrange
    // Set up ptpConfig
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS;
    double pdelay = 0;
    Score_PtpBooleanType isProvider = true;
    // Act
    Score_PtpReturnType result = Score_PtpInitialize(pdelay, testDomainNumber, isProvider);
    // Assert
    EXPECT_EQ(result, SCORE_PTP_E_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpInitialize_WithisProviderFalse_Success) {
    // Arrange
    // Set up ptpConfig
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS;
    double pdelay = 0;
    Score_PtpBooleanType isProvider = false;
    // Act
    Score_PtpReturnType result = Score_PtpInitialize(pdelay, testDomainNumber, isProvider);
    // Assert
    EXPECT_EQ(result, SCORE_PTP_E_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpDeInitialize_Success) {
    // Arrange
    // Initialize first
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS;
    double pdelay = 0;
    Score_PtpBooleanType isProvider = false;
    Score_PtpReturnType result = Score_PtpInitialize(pdelay, testDomainNumber, isProvider);
    EXPECT_EQ(result, SCORE_PTP_E_OK);
    EXPECT_NE(ptpConfig.timebaseHandle, nullptr);

    // Act
    EXPECT_CALL(ptpCommonMock, TSync_CloseTimebase(ptpConfig.timebaseHandle)).Times(1);
    EXPECT_CALL(ptpCommonMock, TSync_Close()).Times(1);
    Score_PtpDeinitialize();
    // Assert
    EXPECT_EQ(ptpConfig.timebaseHandle, nullptr);
    EXPECT_EQ(consumerData.syncState, SCORE_PTP_STATE_INVALID);
}

TEST_F(ScorePtpCommonTest, Score_PtpConfigure_With_TSYNC_MC_IEEE8021AS_AUTOSAR_AsProvider_Success) {
    // Arrange
    Score_PtpBooleanType isProvider = true;
    const int64_t     syncPeriodMs = 5;
    // Setup ptpConfig
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.message_length = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.domain_number = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.correction_field = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.source_port_identity = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.sequence_id = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.precise_origin_timestamp = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.syncPeriodMs = syncPeriodMs;
    // Act
    Score_PtpReturnType result = Score_PtpConfigure(isProvider);
    // Assert
    uint8_t expect_followupLength = SCORE_PTP_FUP_PACKET_LENGTH_FIXED_AUTOSAR + sizeof(Score_PtpSubtlvTimeSecuredType) +
                                    sizeof(Score_PtpSubtlvStatusType) + sizeof(Score_PtpSubtlvUserdataType);
    uint8_t expect_configSubTlvLength =
        sizeof(Score_PtpSubtlvTimeSecuredType) + sizeof(Score_PtpSubtlvStatusType) + sizeof(Score_PtpSubtlvUserdataType);
    EXPECT_EQ(ptpConfig.configGlobals.followupLength, expect_followupLength);
    EXPECT_EQ(ptpConfig.configGlobals.configSubTlvLength, expect_configSubTlvLength);
    // Sync period remains the same as it was initialized - if it were 0, it would have been initialized to 1
    EXPECT_EQ(ptpConfig.timebaseConfig.syncPeriodMs, syncPeriodMs);
    EXPECT_EQ(result, SCORE_PTP_E_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpConfigure_With_TSYNC_MC_IEEE8021AS_AUTOSAR_AsConsumer_Success) {
    // Arrange
    Score_PtpBooleanType isProvider = false;
    // Setup ptpConfig
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.message_length = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.domain_number = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.correction_field = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.source_port_identity = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.sequence_id = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.precise_origin_timestamp = TSYNC_CRC_SUPPORTED;
    ptpConfig.timebaseConfig.syncPeriodMs = 0;

    // Act
    Score_PtpReturnType result = Score_PtpConfigure(isProvider);
    // Assert
    uint8_t expect_followupLength = SCORE_PTP_FUP_PACKET_LENGTH_FIXED_AUTOSAR + sizeof(Score_PtpSubtlvTimeSecuredType) +
                                    sizeof(Score_PtpSubtlvStatusType) + sizeof(Score_PtpSubtlvUserdataType);
    uint8_t expect_configSubTlvLength =
        sizeof(Score_PtpSubtlvTimeSecuredType) + sizeof(Score_PtpSubtlvStatusType) + sizeof(Score_PtpSubtlvUserdataType);
    EXPECT_EQ(ptpConfig.configGlobals.followupLength, expect_followupLength);
    EXPECT_EQ(ptpConfig.configGlobals.configSubTlvLength, expect_configSubTlvLength);
    EXPECT_EQ(ptpConfig.timebaseConfig.syncPeriodMs, SCORE_PTP_DEFAULT_SYNC_PERIOD);
    EXPECT_EQ(result, SCORE_PTP_E_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpConfigure_UpdateNoCrcFlag_Success) {
    // Arrange
    Score_PtpBooleanType isProvider = false;
    // Setup ptpConfig
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_VALIDATED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.message_length = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.domain_number = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.correction_field = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.source_port_identity = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.sequence_id = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.precise_origin_timestamp = TSYNC_CRC_NOT_SUPPORTED;

    // Act
    Score_PtpReturnType result = Score_PtpConfigure(isProvider);
    // Assert
    uint8_t expect_followupLength = SCORE_PTP_FUP_PACKET_LENGTH_FIXED_AUTOSAR + sizeof(Score_PtpSubtlvTimeSecuredType) +
                                    sizeof(Score_PtpSubtlvStatusType) + sizeof(Score_PtpSubtlvUserdataType);
    uint8_t expect_configSubTlvLength =
        sizeof(Score_PtpSubtlvTimeSecuredType) + sizeof(Score_PtpSubtlvStatusType) + sizeof(Score_PtpSubtlvUserdataType);
    EXPECT_EQ(ptpConfig.configGlobals.followupLength, expect_followupLength);
    EXPECT_EQ(ptpConfig.configGlobals.configSubTlvLength, expect_configSubTlvLength);
    EXPECT_EQ(result, SCORE_PTP_E_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpConfigure_With_NoSubTLVSupported_AsProvider_Success) {
    // Arrange
    Score_PtpBooleanType isProvider = true;
    // Setup ptpConfig
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.crcSupport = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.message_length = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.domain_number = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.correction_field = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.source_port_identity = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.sequence_id = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.precise_origin_timestamp = TSYNC_CRC_NOT_SUPPORTED;

    // Act
    Score_PtpReturnType result = Score_PtpConfigure(isProvider);
    // Assert
    EXPECT_EQ(ptpConfig.configGlobals.followupLength, SCORE_PTP_FUP_PACKET_LENGTH_FIXED_AUTOSAR);
    EXPECT_EQ(result, SCORE_PTP_E_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpConfigure_With_NoSubTLVSupported_AsConsumer_Success) {
    // Arrange
    Score_PtpBooleanType isProvider = false;
    // Setup ptpConfig
    ptpConfig.timebaseConfig.messageCompliance = TSYNC_MC_IEEE8021AS_AUTOSAR;
    ptpConfig.timebaseConfig.crcValidation = TSYNC_CRC_NOT_VALIDATED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.message_length = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.domain_number = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.correction_field = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.source_port_identity = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.sequence_id = TSYNC_CRC_NOT_SUPPORTED;
    ptpConfig.timebaseConfig.crcFlags.precise_origin_timestamp = TSYNC_CRC_NOT_SUPPORTED;

    // Act
    Score_PtpReturnType result = Score_PtpConfigure(isProvider);
    // Assert
    EXPECT_EQ(ptpConfig.configGlobals.followupLength, SCORE_PTP_FUP_PACKET_LENGTH_FIXED_AUTOSAR);
    EXPECT_EQ(result, SCORE_PTP_E_OK);
}

TEST_F(ScorePtpCommonTest, Score_PtpExtractFupHeaderInfo_WithNullfollowupHeaderInfo_Failed) {
    // Arrange
    MsgHeaderTag* followupHeaderInfo = nullptr;
    Score_PtpBooleanType isProvider = true;
    // Act
    Score_PtpExtractFupHeaderInfo(followupHeaderInfo, isProvider);
    // Assert
}

TEST_F(ScorePtpCommonTest, Score_PtpExtractFupHeaderInfo_AsProvider_Success) {
    // Arrange
    struct MsgHeaderTag followupHeaderInfo;
    followupHeaderInfo.transportSpecific = 0u;
    followupHeaderInfo.messageType = 1u;
    followupHeaderInfo.reserved0 = 1u;
    followupHeaderInfo.versionPTP = 1u;
    followupHeaderInfo.messageLength = 32u;
    followupHeaderInfo.domainNumber = static_cast<UInteger8>(testDomainNumber);
    followupHeaderInfo.flagField0 = 0u;
    followupHeaderInfo.flagField1 = 1u;
    followupHeaderInfo.correctionField.msb = 1u;
    followupHeaderInfo.correctionField.lsb = 0u;
    followupHeaderInfo.sourcePortIdentity.portNumber = 32u;
    std::fill(followupHeaderInfo.sourcePortIdentity.clockIdentity,
              followupHeaderInfo.sourcePortIdentity.clockIdentity + CLOCK_IDENTITY_LENGTH, 0);
    followupHeaderInfo.sequenceId = 1u;
    followupHeaderInfo.controlField = 0u;
    followupHeaderInfo.logMessageInterval = 1u;

    Score_PtpBooleanType isProvider = true;
    // Act
    Score_PtpExtractFupHeaderInfo(&followupHeaderInfo, isProvider);
    // Assert
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.transportSpecific, followupHeaderInfo.transportSpecific);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.messageType, followupHeaderInfo.messageType);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.reserved0, followupHeaderInfo.reserved0);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.versionPtp, followupHeaderInfo.versionPTP);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.messageLength, followupHeaderInfo.messageLength);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.domainNumber, followupHeaderInfo.domainNumber);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.flagField0, followupHeaderInfo.flagField0);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.flagField1, followupHeaderInfo.flagField1);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.sequenceId, followupHeaderInfo.sequenceId);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.controlField, followupHeaderInfo.controlField);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.logMessageInterval, followupHeaderInfo.logMessageInterval);
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.controlField,
              ((followupHeaderInfo.correctionField.msb << 32) | (followupHeaderInfo.correctionField.lsb)));
    EXPECT_EQ(providerData.followUpMsg.followupHeaderInfo.sourcePortIdentity.portNumber,
              followupHeaderInfo.sourcePortIdentity.portNumber);
    for (int i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
        EXPECT_EQ(followupHeaderInfo.sourcePortIdentity.clockIdentity[i],
                  providerData.followUpMsg.followupHeaderInfo.sourcePortIdentity.clockIdentity[i]);
    }
}

TEST_F(ScorePtpCommonTest, Score_PtpExtractFupHeaderInfo_AsConsumer_Success) {
    // Arrange
    struct MsgHeaderTag followupHeaderInfo;
    followupHeaderInfo.transportSpecific = 0u;
    followupHeaderInfo.messageType = 1u;
    followupHeaderInfo.reserved0 = 1u;
    followupHeaderInfo.versionPTP = 1u;
    followupHeaderInfo.messageLength = 32u;
    followupHeaderInfo.domainNumber = static_cast<UInteger8>(testDomainNumber);
    followupHeaderInfo.flagField0 = 0u;
    followupHeaderInfo.flagField1 = 1u;
    followupHeaderInfo.correctionField.msb = 1u;
    followupHeaderInfo.correctionField.lsb = 0u;
    followupHeaderInfo.sourcePortIdentity.portNumber = 32u;
    std::fill(followupHeaderInfo.sourcePortIdentity.clockIdentity,
              followupHeaderInfo.sourcePortIdentity.clockIdentity + CLOCK_IDENTITY_LENGTH, 0);
    followupHeaderInfo.sequenceId = 1u;
    followupHeaderInfo.controlField = 0u;
    followupHeaderInfo.logMessageInterval = 1u;

    Score_PtpBooleanType isProvider = false;
    // Act
    Score_PtpExtractFupHeaderInfo(&followupHeaderInfo, isProvider);
    // Assert
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.transportSpecific, followupHeaderInfo.transportSpecific);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.messageType, followupHeaderInfo.messageType);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.reserved0, followupHeaderInfo.reserved0);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.versionPtp, followupHeaderInfo.versionPTP);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.messageLength, followupHeaderInfo.messageLength);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.domainNumber, followupHeaderInfo.domainNumber);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.flagField0, followupHeaderInfo.flagField0);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.flagField1, followupHeaderInfo.flagField1);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.sequenceId, followupHeaderInfo.sequenceId);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.controlField, followupHeaderInfo.controlField);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.logMessageInterval, followupHeaderInfo.logMessageInterval);
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.controlField,
              ((followupHeaderInfo.correctionField.msb << 32) | (followupHeaderInfo.correctionField.lsb)));
    EXPECT_EQ(consumerData.followUpMsg.followupHeaderInfo.sourcePortIdentity.portNumber,
              followupHeaderInfo.sourcePortIdentity.portNumber);
    for (int i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
        EXPECT_EQ(followupHeaderInfo.sourcePortIdentity.clockIdentity[i],
                  consumerData.followUpMsg.followupHeaderInfo.sourcePortIdentity.clockIdentity[i]);
    }
}

}  // namespace ptp_daemon_common_ut
}  // namespace testing