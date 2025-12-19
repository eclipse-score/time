/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>
#include "inc/score_ptp_consumer.h"
#include "score_ptp_consumer_mock.h"

extern Score_PtpConsumerDataType consumerData;

Score_PtpConfigType ptpConfig = {};

// Create a global instance of the PtpConsumerMock class
PtpConsumerMock ptpConsumerMock;

// Define a mock implementation of TSync_GetCurrentVirtualLocalTime
TSync_ReturnType TSync_GetCurrentVirtualLocalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                                  TSync_VirtualLocalTimeType* localTimePtr) {
    // Forward the call to the mock instance
    return ptpConsumerMock.TSync_GetCurrentVirtualLocalTime(timeBaseHandle, localTimePtr);
}

// Define a mock implementation of TSync_BusSetGlobalTime
TSync_ReturnType TSync_BusSetGlobalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                         const TSync_TimeStampType* globalTimePtr,
                                         const TSync_UserDataType* userDataPtr,
                                         const TSync_MeasurementType* measureDataPtr,
                                         const TSync_VirtualLocalTimeType* localTimePtr) {
    // Forward the call to the mock instance
    return ptpConsumerMock.TSync_BusSetGlobalTime(timeBaseHandle, globalTimePtr, userDataPtr, measureDataPtr, localTimePtr);
}

// Define a mock implementation of Score_PtpReadSubTlvFromBuffer
Score_PtpBooleanType Score_PtpReadSubTlvFromBuffer(const uint8_t *autosarTlvBufferIn) {
    // Forward the call to the mock instance
    return ptpConsumerMock.Score_PtpReadSubTlvFromBuffer(autosarTlvBufferIn);
}

class ScorePtpConsumerTest : public ::testing::Test {
protected:
    PtpConsumerMock ptpConsumer;

    void SetUp() override {
        // Reset consumerData before each test
        memset(&consumerData, 0, sizeof(Score_PtpConsumerDataType));
    }

    void TearDown() override {
        // Verify and clear mock expectations
        testing::Mock::VerifyAndClearExpectations(&ptpConsumerMock);
    }
};

namespace testing {
namespace ptp_daemon_consumer_ut {

TEST_F(ScorePtpConsumerTest, Score_PtpReadFupInfoFromBuffer_SyncStateReceived_SubTlvReadSuccess) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_SYNC_RCVD;
    TSync_TimeStampType correctionField = {TIMEBASE_STATUS_BIT_GLOBAL_SYNC, SCORE_PTP_NANOSEC, 1u, 0u};
    Score_PtpTimeStampType originTime = {1u, 0u, SCORE_PTP_NANOSEC};
    uint8_t fupInfoIn = 0x01;

    uint8_t corrTimeIn[sizeof(TSync_TimeStampType)];
    uint8_t originTimeIn[sizeof(Score_PtpTimeStampType)];

    // Copy the structs into byte arrays
    memcpy(corrTimeIn, &correctionField, sizeof(TSync_TimeStampType));
    memcpy(originTimeIn, &originTime, sizeof(Score_PtpTimeStampType));

    EXPECT_CALL(ptpConsumerMock, Score_PtpReadSubTlvFromBuffer(_))
        .WillOnce(Return(SCORE_PTP_TRUE));

    // Act
    Score_PtpReadFupInfoFromBuffer(originTimeIn, corrTimeIn, &fupInfoIn);

    // Assert
    EXPECT_EQ(consumerData.correctionField.ts.secondsHi, correctionField.secondsHi);
    EXPECT_EQ(consumerData.correctionField.ts.seconds, correctionField.seconds);
    EXPECT_EQ(consumerData.correctionField.ts.nanoseconds, correctionField.nanoseconds);
    EXPECT_EQ(consumerData.correctionField.sign, SCORE_PTP_POSITIVE);

    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi, originTime.secondsHi);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.seconds, originTime.seconds);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds, originTime.nanoseconds);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.sign, SCORE_PTP_POSITIVE);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadFupInfoFromBuffer_SyncStateReceived_SubTlvReadFailure) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_SYNC_RCVD;
    TSync_TimeStampType correctionField = {TIMEBASE_STATUS_BIT_GLOBAL_SYNC, SCORE_PTP_NANOSEC, 1u, 0u};
    Score_PtpTimeStampType originTime = {1u, 0u, SCORE_PTP_NANOSEC};
    uint8_t fupInfoIn = 0x01;

    uint8_t corrTimeIn[sizeof(TSync_TimeStampType)];
    uint8_t originTimeIn[sizeof(Score_PtpTimeStampType)];

    // Copy the structs into byte arrays
    memcpy(corrTimeIn, &correctionField, sizeof(TSync_TimeStampType));
    memcpy(originTimeIn, &originTime, sizeof(Score_PtpTimeStampType));

    EXPECT_CALL(ptpConsumerMock, Score_PtpReadSubTlvFromBuffer(_))
        .WillOnce(Return(SCORE_PTP_FALSE));

    // Act
    Score_PtpReadFupInfoFromBuffer(originTimeIn, corrTimeIn, &fupInfoIn);

    // Assert
    EXPECT_EQ(consumerData.correctionField.ts.secondsHi, correctionField.secondsHi);
    EXPECT_EQ(consumerData.correctionField.ts.seconds, correctionField.seconds);
    EXPECT_EQ(consumerData.correctionField.ts.nanoseconds, correctionField.nanoseconds);
    EXPECT_EQ(consumerData.correctionField.sign, SCORE_PTP_POSITIVE);

    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi, originTime.secondsHi);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.seconds, originTime.seconds);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds, originTime.nanoseconds);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.sign, SCORE_PTP_POSITIVE);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadFupInfoFromBuffer_NullInputs_Failure) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_SYNC_RCVD;
    TSync_TimeStampType correctionField = {TIMEBASE_STATUS_BIT_GLOBAL_SYNC, SCORE_PTP_NANOSEC, 1u, 0u};

    // NULL for fupInfoIn and originTimeIn
    uint8_t* fupInfoIn = NULL;
    uint8_t* originTimeIn = NULL;
    uint8_t corrTimeIn[sizeof(TSync_TimeStampType)];

    // Copy the struct into the byte array
    memcpy(corrTimeIn, &correctionField, sizeof(TSync_TimeStampType));

    // Act
    Score_PtpReadFupInfoFromBuffer(originTimeIn, corrTimeIn, fupInfoIn);

    // Assert
    EXPECT_EQ(consumerData.correctionField.ts.secondsHi, 0);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi, 0);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadFupInfoFromBuffer_SyncStateFupReceived_Failure) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_FUP_RCVD;
    TSync_TimeStampType correctionField = {TIMEBASE_STATUS_BIT_GLOBAL_SYNC, SCORE_PTP_NANOSEC, 1u, 0u};
    Score_PtpTimeStampType originTime = {1u, 0u, SCORE_PTP_NANOSEC};
    uint8_t fupInfoIn = 0x01;

    uint8_t corrTimeIn[sizeof(TSync_TimeStampType)];
    uint8_t originTimeIn[sizeof(Score_PtpTimeStampType)];

    // Copy the structs into byte arrays
    memcpy(corrTimeIn, &correctionField, sizeof(TSync_TimeStampType));
    memcpy(originTimeIn, &originTime, sizeof(Score_PtpTimeStampType));

    // Act
    Score_PtpReadFupInfoFromBuffer(originTimeIn, corrTimeIn, &fupInfoIn);

    // Assert
    EXPECT_EQ(consumerData.correctionField.ts.secondsHi, 0);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi, 0);

    EXPECT_NE(consumerData.correctionField.sign, SCORE_PTP_POSITIVE);
    EXPECT_NE(consumerData.followUpMsg.preciseOriginTimestamp.sign, SCORE_PTP_POSITIVE);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadFupInfoFromBuffer_InvalidSyncState_Failure) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_INVALID;
    TSync_TimeStampType correctionField = {TIMEBASE_STATUS_BIT_GLOBAL_SYNC, SCORE_PTP_NANOSEC, 1u, 0u};
    Score_PtpTimeStampType originTime = {1u, 0u, SCORE_PTP_NANOSEC};

    uint8_t corrTimeIn[sizeof(TSync_TimeStampType)];
    uint8_t originTimeIn[sizeof(Score_PtpTimeStampType)];
    uint8_t fupInfoIn = 0x01;

    // Copy the structs into byte arrays
    memcpy(corrTimeIn, &correctionField, sizeof(TSync_TimeStampType));
    memcpy(originTimeIn, &originTime, sizeof(Score_PtpTimeStampType));

    // Act
    Score_PtpReadFupInfoFromBuffer(originTimeIn, corrTimeIn, &fupInfoIn);

    // Assert
    EXPECT_EQ(consumerData.correctionField.ts.secondsHi, 0);
    EXPECT_EQ(consumerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi, 0);

    EXPECT_NE(consumerData.correctionField.sign, SCORE_PTP_POSITIVE);
    EXPECT_NE(consumerData.followUpMsg.preciseOriginTimestamp.sign, SCORE_PTP_POSITIVE);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadIngressVlt_T1_Success) {
    // Arrange
    TSync_VirtualLocalTimeType virtualLocalTime = {1u, 1u};

    // Set up the mock behavior
    EXPECT_CALL(ptpConsumerMock, TSync_GetCurrentVirtualLocalTime(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(virtualLocalTime), Return(E_OK)));

    // Act
    Score_PtpReadIngressVlt(SCORE_PTP_INGRESS_T1);

    // Assert
    EXPECT_EQ(consumerData.t1Vlt.nanosecondsHi, virtualLocalTime.nanosecondsHi);
    EXPECT_EQ(consumerData.t1Vlt.nanosecondsLo, virtualLocalTime.nanosecondsLo);
    EXPECT_EQ(consumerData.syncState, SCORE_PTP_STATE_SYNC_RCVD);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadIngressVlt_T2_Success) {
    // Arrange
    TSync_VirtualLocalTimeType virtualLocalTime = {1u, 1u};

    // Set up the mock behavior
    EXPECT_CALL(ptpConsumerMock, TSync_GetCurrentVirtualLocalTime(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(virtualLocalTime), Return(E_OK)));

    // Act
    Score_PtpReadIngressVlt(SCORE_PTP_INGRESS_T2);

    // Assert
    EXPECT_EQ(consumerData.t2Vlt.nanosecondsHi, virtualLocalTime.nanosecondsHi);
    EXPECT_EQ(consumerData.t2Vlt.nanosecondsLo, virtualLocalTime.nanosecondsLo);
    EXPECT_EQ(consumerData.syncState, SCORE_PTP_STATE_FUP_RCVD);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadIngressVlt_Failure) {
    // Arrange
    // Set up the mock behavior to return E_NOT_OK
    EXPECT_CALL(ptpConsumerMock, TSync_GetCurrentVirtualLocalTime)
        .WillOnce(Return(E_NOT_OK));

    // Act
    Score_PtpReadIngressVlt(SCORE_PTP_INGRESS_T1);

    // Assert
    EXPECT_EQ(consumerData.syncState, SCORE_PTP_STATE_INVALID);
    EXPECT_EQ(consumerData.t1Vlt.nanosecondsHi, 0);
    EXPECT_EQ(consumerData.t1Vlt.nanosecondsLo, 0);
}

TEST_F(ScorePtpConsumerTest, Score_PtpReadIngressVlt_InvalidIngress) {

    // Arrange & Act
    Score_PtpReadIngressVlt(SCORE_PTP_INGRESS_UNDEFINED);

    // Assert
    EXPECT_EQ(consumerData.t1Vlt.nanosecondsHi, 0);
    EXPECT_EQ(consumerData.t1Vlt.nanosecondsLo, 0);
    EXPECT_EQ(consumerData.t2Vlt.nanosecondsHi, 0);
    EXPECT_EQ(consumerData.t2Vlt.nanosecondsLo, 0);
}

TEST_F(ScorePtpConsumerTest, Score_PtpBusSetGlobalTime_FUPReceived_Success) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_FUP_RCVD;

    Score_PtpSignedTimeStampType syncTimeStamp = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType timeStampDiff = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};

    uint64_t syncRxTimeStamp = 0u;
    uint64_t fupRxTimeStamp = 0u;

    TSync_TimeStampType globalTime = {0u, 0u, 0u, 0u};
    Score_PtpSignedTimeStampType finalGlobalTime = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};

    TSync_UserDataType userdata = {0u, 0u, 0u, 0u};

    TSync_MeasurementType measureData = {0u};
    TSync_VirtualLocalTimeType localTime = {0u, 0u};

    EXPECT_CALL(ptpConsumerMock, TSync_BusSetGlobalTime(_, _, _, _, _))
        .WillOnce(Return(E_OK));

    // Act
    Score_PtpBusSetGlobalTime();

    // Assert
    EXPECT_EQ(syncTimeStamp.ts.nanoseconds, timeStampDiff.ts.nanoseconds);
    EXPECT_EQ(syncTimeStamp.ts.seconds, timeStampDiff.ts.seconds);
    EXPECT_EQ(syncTimeStamp.ts.secondsHi, timeStampDiff.ts.secondsHi);

    // Expected syncRxTimeStamp: Combine t1Vlt.nanosecondsHi and t1Vlt.nanosecondsLo
    uint64_t expectedSyncRxTimeStamp = ((uint64_t)consumerData.t1Vlt.nanosecondsHi << 32u) |
                                    consumerData.t1Vlt.nanosecondsLo;

    // Expected fupRxTimeStamp: Combine t2Vlt.nanosecondsHi and t2Vlt.nanosecondsLo
    uint64_t expectedFupRxTimeStamp = ((uint64_t)consumerData.t2Vlt.nanosecondsHi << 32u) |
                                    consumerData.t2Vlt.nanosecondsLo;

    EXPECT_EQ(syncRxTimeStamp, expectedSyncRxTimeStamp);
    EXPECT_EQ(fupRxTimeStamp, expectedFupRxTimeStamp);

    EXPECT_EQ(measureData.pathDelay, ptpConfig.pdelay.ts.nanoseconds);

    EXPECT_EQ(localTime.nanosecondsLo, consumerData.t2Vlt.nanosecondsLo);
    EXPECT_EQ(localTime.nanosecondsHi, consumerData.t2Vlt.nanosecondsHi);

    EXPECT_EQ(globalTime.nanoseconds, finalGlobalTime.ts.nanoseconds);
    EXPECT_EQ(globalTime.seconds, finalGlobalTime.ts.seconds);
    EXPECT_EQ(globalTime.secondsHi, finalGlobalTime.ts.secondsHi);

    EXPECT_EQ(userdata.userDataLength, consumerData.followUpMsg.subTlv.userdata.userDataLength);
    EXPECT_EQ(userdata.userByte0, consumerData.followUpMsg.subTlv.userdata.userByte0);
    EXPECT_EQ(userdata.userByte1, consumerData.followUpMsg.subTlv.userdata.userByte1);
    EXPECT_EQ(userdata.userByte2, consumerData.followUpMsg.subTlv.userdata.userByte2);
}

TEST_F(ScorePtpConsumerTest, Score_PtpBusSetGlobalTime_FUPReceived_Failure) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_FUP_RCVD;

    Score_PtpSignedTimeStampType syncTimeStamp = {{0u, 0u, 0u, 0u}, SCORE_PTP_POSITIVE};
    Score_PtpSignedTimeStampType timeStampDiff = {{1u, 1u, 1u, 1u}, SCORE_PTP_POSITIVE};

    TSync_TimeStampType globalTime = {0u, 0u, 0u, 0u};
    Score_PtpSignedTimeStampType finalGlobalTime = {{1u, 1u, 1u, 1u}, SCORE_PTP_POSITIVE};

    TSync_MeasurementType measureData = {1u};

    EXPECT_CALL(ptpConsumerMock, TSync_BusSetGlobalTime(_, _, _, _, _))
        .WillOnce(Return(E_NOT_OK));

    // Act
    Score_PtpBusSetGlobalTime();

    // Assert
    EXPECT_NE(syncTimeStamp.ts.nanoseconds, timeStampDiff.ts.nanoseconds);
    EXPECT_NE(syncTimeStamp.ts.seconds, timeStampDiff.ts.seconds);
    EXPECT_NE(syncTimeStamp.ts.secondsHi, timeStampDiff.ts.secondsHi);

    EXPECT_NE(measureData.pathDelay, ptpConfig.pdelay.ts.nanoseconds);

    EXPECT_NE(globalTime.nanoseconds, finalGlobalTime.ts.nanoseconds);
    EXPECT_NE(globalTime.seconds, finalGlobalTime.ts.seconds);
    EXPECT_NE(globalTime.secondsHi, finalGlobalTime.ts.secondsHi);
}

TEST_F(ScorePtpConsumerTest, Score_PtpBusSetGlobalTime_SyncStateNotFUP_NoGlobalTimeSet) {
    // Arrange
    consumerData.syncState = SCORE_PTP_STATE_SYNC_RCVD;

    TSync_TimeStampType globalTime = {0u, 0u, 0u, 0u};
    TSync_UserDataType userdata = {0u, 0u, 0u, 0u};
    TSync_MeasurementType measureData = {0u};

    // Set up the mock behavior to return E_NOT_OK
    EXPECT_CALL(ptpConsumerMock, TSync_GetCurrentVirtualLocalTime)
        .WillOnce(Return(E_NOT_OK));

    // Act
    Score_PtpBusSetGlobalTime();

    // Assert
    EXPECT_EQ(consumerData.syncState, SCORE_PTP_STATE_INVALID);

    EXPECT_EQ(globalTime.nanoseconds, 0);
    EXPECT_EQ(userdata.userDataLength, 0);
    EXPECT_EQ(measureData.pathDelay, 0);
}

} // namespace ptp_daemon_consumer_ut
} // namespace testing
