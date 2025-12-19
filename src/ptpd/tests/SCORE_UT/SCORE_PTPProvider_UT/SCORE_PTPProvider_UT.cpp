/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score_ptp_provider_mock.h"

#include <gtest/gtest.h>
#include <thread>

using ::testing::_;
using ::testing::Return;

extern Score_PtpProviderDataType providerData;

Score_PtpConfigType ptpConfig = {};

// Create a global instance of the PtpProviderMock class
PtpProviderMock ptpProviderMock;

// Define a mock implementation of TSync_GetCurrentVirtualLocalTime
TSync_ReturnType TSync_GetCurrentVirtualLocalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                                  TSync_VirtualLocalTimeType* localTimePtr) {
    // Forward the call to the mock instance
    return ptpProviderMock.TSync_GetCurrentVirtualLocalTime(timeBaseHandle, localTimePtr);
}

// Define a mock implementation of TSync_BusGetGlobalTime
TSync_ReturnType TSync_BusGetGlobalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                        TSync_TimeStampType* globalTimePtr,
                                        TSync_UserDataType* userDataPtr,
                                        TSync_MeasurementType* measureDataPtr,
                                        TSync_VirtualLocalTimeType* localTimePtr) {
    // Forward the call to the mock instance
    return ptpProviderMock.TSync_BusGetGlobalTime(timeBaseHandle, globalTimePtr, userDataPtr, measureDataPtr, localTimePtr);
}

class ScorePtpProviderTest : public ::testing::Test {
protected:
     PtpProviderMock ptpProvider;
     void SetUp() override {

        // Reset providerData before each test
        memset(&providerData, 0, sizeof(Score_PtpProviderDataType));
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(&ptpProviderMock);
    }
};

namespace testing {
namespace ptp_daemon_provider_ut {


TEST_F(ScorePtpProviderTest, Score_PtpReadEgressVlt_TSync_GetCurrentVirtualLocalTime_Failure) {
    // Arrange
    // Set up the mock behavior
     EXPECT_CALL(ptpProviderMock, TSync_GetCurrentVirtualLocalTime)
        .WillOnce(Return(E_NOT_OK));

    // Set some default values
    providerData.t4Vlt.nanosecondsHi = 0xDEAD;
    providerData.t4Vlt.nanosecondsLo = 0xDEAD;

    // Act
    Score_PtpReadEgressVlt(SCORE_PTP_EGRESS_T4);

    // Assert
    EXPECT_EQ(providerData.t4Vlt.nanosecondsHi, 0xDEAD);
    EXPECT_EQ(providerData.t4Vlt.nanosecondsLo, 0xDEAD);
}

TEST_F(ScorePtpProviderTest, Score_PtpReadEgressVlt_TSync_GetCurrentVirtualLocalTime_T4_Success) {

    // Arrange
    TSync_VirtualLocalTimeType virtualLocalTime = {0xC001, 0xCAFE};

    EXPECT_CALL(ptpProviderMock, TSync_GetCurrentVirtualLocalTime(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(virtualLocalTime), Return(E_OK)));

    providerData.t4Vlt.nanosecondsHi = 0xDEAD;
    providerData.t4Vlt.nanosecondsLo = 0xDEAD;

    // Act
    Score_PtpReadEgressVlt(SCORE_PTP_EGRESS_T4);

    // Assert
    EXPECT_EQ(providerData.t4Vlt.nanosecondsLo, 0xC001);
    EXPECT_EQ(providerData.t4Vlt.nanosecondsHi, 0xCAFE);
}

TEST_F(ScorePtpProviderTest, Score_PtpReadEgressVlt_TSync_GetCurrentVirtualLocalTime_T0_Success) {

    // Arrange
    TSync_VirtualLocalTimeType virtualLocalTime = {0xC001, 0xCAFE};

    // Arrange
    EXPECT_CALL(ptpProviderMock, TSync_GetCurrentVirtualLocalTime(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(virtualLocalTime), Return(E_OK)));

    providerData.t4Vlt.nanosecondsHi = 0xDEAD;
    providerData.t4Vlt.nanosecondsLo = 0xDEAD;

    // Act
    Score_PtpReadEgressVlt(SCORE_PTP_EGRESS_T0);

    // Assert
    EXPECT_EQ(providerData.t4Vlt.nanosecondsLo, 0xDEAD);
    EXPECT_EQ(providerData.t4Vlt.nanosecondsHi, 0xDEAD);
}

TEST_F(ScorePtpProviderTest, Score_PtpBusGetGlobalTime_TSync_BusGetGlobalTime_Failure) {
    // Arrange
    // Set up the mock behavior
     EXPECT_CALL(ptpProviderMock, TSync_BusGetGlobalTime(_,_,_,_,_))
        .WillOnce(Return(E_NOT_OK));

    // Set some default values
    providerData.t0Vlt.nanosecondsHi = 0xDEAD;
    providerData.t0Vlt.nanosecondsLo = 0xDEAD;

    // Act
    Score_PtpBusGetGlobalTime();

    // Assert
    EXPECT_EQ(providerData.t0Vlt.nanosecondsHi, 0xDEAD);
    EXPECT_EQ(providerData.t0Vlt.nanosecondsLo, 0xDEAD);
}

TEST_F(ScorePtpProviderTest, Score_PtpBusGetGlobalTime_TSync_BusGetGlobalTime_Success) {

    // Arrange
    EXPECT_CALL(ptpProviderMock, TSync_BusGetGlobalTime(_,_,_,_,_))
        .WillOnce(Return(E_OK));

    providerData.t0Vlt.nanosecondsHi = 0xDEAD;
    providerData.t0Vlt.nanosecondsLo = 0xDEAD;

    // Act
    Score_PtpBusGetGlobalTime();

    // Assert
    EXPECT_EQ(providerData.t0Vlt.nanosecondsLo, 0);
    EXPECT_EQ(providerData.t0Vlt.nanosecondsHi, 0);

    // Arrange
    TSync_TimeStampType globalTime = {0x00 ,0xA, 0xB, 0xC};
    TSync_UserDataType userdata   = {0x3,  0xB0, 0xB1, 0xB2};
    TSync_VirtualLocalTimeType virtualLocalTime = {0xC001, 0xCAFE};

    EXPECT_CALL(ptpProviderMock, TSync_BusGetGlobalTime(_,_,_,_,_))
        .WillOnce(DoAll(SetArgPointee<1>(globalTime),
                        SetArgPointee<2>(userdata),
                        SetArgPointee<4>(virtualLocalTime), Return(E_OK)));

    // Act
    Score_PtpBusGetGlobalTime();

    // Assert
    EXPECT_EQ(providerData.t0Vlt.nanosecondsLo, 0xC001);
    EXPECT_EQ(providerData.t0Vlt.nanosecondsHi, 0xCAFE);
    EXPECT_EQ(providerData.t0GlobalTime.nanoseconds, 0xA);
    EXPECT_EQ(providerData.t0GlobalTime.seconds, 0xB);
    EXPECT_EQ(providerData.t0GlobalTime.secondsHi, 0xC);
    EXPECT_EQ(providerData.followUpMsg.subTlv.userdata.userDataLength, 0x3);
    EXPECT_EQ(providerData.followUpMsg.subTlv.userdata.userByte0, 0xB0);
    EXPECT_EQ(providerData.followUpMsg.subTlv.userdata.userByte1, 0xB1);
    EXPECT_EQ(providerData.followUpMsg.subTlv.userdata.userByte2, 0xB2);

    //Arrange
    EXPECT_CALL(ptpProviderMock, TSync_BusGetGlobalTime(_,_,_,_,_))
        .WillOnce(Return(E_OK));

    // Act
    Score_PtpBusGetGlobalTime();
}

TEST_F(ScorePtpProviderTest, Score_PtpProviderComputeOriginTime_Success) {
    // Arrange
    /* Set Current Virtual Local Time to 4000000000ns which means 4 seconds */
    TSync_VirtualLocalTimeType virtualLocalTime = {0xEE6B2800, 0x00};

    // Current Virtual Local Time
    EXPECT_CALL(ptpProviderMock, TSync_GetCurrentVirtualLocalTime(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(virtualLocalTime), Return(E_OK)));

    // Previous Global Time
    providerData.t0Vlt.nanosecondsHi = 0x00;
    providerData.t0Vlt.nanosecondsLo = 0x00;

    providerData.t0GlobalTime.secondsHi = 0x00;
    providerData.t0GlobalTime.seconds   = 0x00;
    providerData.t0GlobalTime.nanoseconds = 0x00;

    // Current GlobalTime = PreviousGlobalTime + (CurrentVLT - PreviousVLT);
    TSync_TimeStampType globalTimeExpected;
    globalTimeExpected.seconds = 4;
    globalTimeExpected.secondsHi = 0;
    globalTimeExpected.nanoseconds = 0;

    // Act
    Score_PtpProviderComputeOriginTime();

    // Assert
    EXPECT_EQ(globalTimeExpected.seconds, providerData.followUpMsg.preciseOriginTimestamp.ts.seconds);
    EXPECT_EQ(globalTimeExpected.secondsHi, providerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi);
    EXPECT_EQ(globalTimeExpected.nanoseconds, providerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds);
}

TEST_F(ScorePtpProviderTest, Score_PtpWriteOriginTimeToBuffer_Success) {

    // Arrange
    uint8_t originTimeBuffer[12];

    providerData.followUpMsg.preciseOriginTimestamp.ts.seconds     = 0xA;
    providerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi   = 0xB;
    providerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds = 0xC;

    //Act
    Score_PtpWriteOriginTimeToBuffer(&originTimeBuffer[0],sizeof(Score_PtpTimeStampType));

    Score_PtpTimeStampType originTimeStamp;
    memcpy((void*)(&originTimeStamp), (void*)(&originTimeBuffer[0]), sizeof(Score_PtpTimeStampType));

    // Assert
    EXPECT_EQ(providerData.followUpMsg.preciseOriginTimestamp.ts.seconds, 0xA);
    EXPECT_EQ(providerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi, 0xB);
    EXPECT_EQ(providerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds, 0xC);
}

TEST_F(ScorePtpProviderTest, Score_PtpWriteOriginTimeToBuffer_OnInvalidBufferSize_Failure) {

    // Arrange
    uint8_t originTimeBuffer[12];

    providerData.followUpMsg.preciseOriginTimestamp.ts.seconds     = 0xA;
    providerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi   = 0xB;
    providerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds = 0xC;

    //Act
    Score_PtpWriteOriginTimeToBuffer(&originTimeBuffer[0],1);

    // Assert
    EXPECT_EQ(providerData.followUpMsg.preciseOriginTimestamp.ts.seconds, 0xA);
    EXPECT_EQ(providerData.followUpMsg.preciseOriginTimestamp.ts.secondsHi, 0xB);
    EXPECT_EQ(providerData.followUpMsg.preciseOriginTimestamp.ts.nanoseconds, 0xC);
}


TEST_F(ScorePtpProviderTest, Score_PtpTransmitGlobalTimeCallback_Domain_Id_Invalid) {
    // Arrange
    // Set the configured domain id
    ptpConfig.timebaseId = 1;
    TSync_SynchronizedTimeBaseType invalidDomainId = 12;

    //Act
    TSync_ReturnType returnValue = Score_PtpTransmitGlobalTimeCallback(invalidDomainId);

    // Assert
    EXPECT_EQ(returnValue, E_NOT_OK);
}

TEST_F(ScorePtpProviderTest, Score_PtpTransmitGlobalTimeCallback_Success) {
    // Arrange
    // Set the configured domain id
    const TSync_SynchronizedTimeBaseType validDomainId = 1;
    ptpConfig.timebaseId = validDomainId;

    // Act
    TSync_ReturnType returnValue = Score_PtpTransmitGlobalTimeCallback(validDomainId);

    // Assert
    EXPECT_EQ(returnValue, E_OK);
    EXPECT_EQ(providerData.immediateTimeTransmission, SCORE_PTP_TRUE);
}

TEST_F(ScorePtpProviderTest, Score_PtpTransmitGlobalTimeCallback_OnCallFromDifferentThread_FlagChangeIsVisible) {
    //Arrange
    providerData.immediateTimeTransmission = SCORE_PTP_FALSE;
    const TSync_SynchronizedTimeBaseType validDomainId = 1;
    ptpConfig.timebaseId = validDomainId;    
    EXPECT_EQ(providerData.immediateTimeTransmission, SCORE_PTP_FALSE);
    
    std::thread trigger_thread([&]() {
        EXPECT_EQ(providerData.immediateTimeTransmission, SCORE_PTP_FALSE);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        Score_PtpTransmitGlobalTimeCallback(validDomainId);
    }); 

    trigger_thread.join();

    // Assert
    EXPECT_EQ(providerData.immediateTimeTransmission, SCORE_PTP_TRUE);
}

TEST_F(ScorePtpProviderTest, Score_PtpIsImmediateTimeTriggerTrue_Success) {
    //Arrange
    providerData.immediateTimeTransmission = SCORE_PTP_TRUE;

    // Act
    bool triggerValue = Score_PtpIsImmediateTimeTriggerTrue();

    // Assert
    EXPECT_EQ(triggerValue, SCORE_PTP_TRUE);
}

TEST_F(ScorePtpProviderTest, Score_PtpResetImmediateTimeTrigger_Success) {
    // Arrange
    providerData.immediateTimeTransmission = SCORE_PTP_TRUE;

    // Act
    Score_PtpResetImmediateTimeTrigger();

    // Assert
    EXPECT_EQ(providerData.immediateTimeTransmission, SCORE_PTP_FALSE);
}

} // namespace ptp_daemon_provider_ut
} // namespace testing
