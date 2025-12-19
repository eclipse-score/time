/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "SignalMock.h"
#include <memory>

#define private public
#include "HouseKeeping.h"
#undef private
#undef raise
#undef sigemptyset
#undef sigaction
using  score::time::HouseKeeping;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::HasSubstr;

extern void SignalHandler(int signal);

namespace testing {
namespace daemon_housekeeping_ut {

class DaemonHouseKeepingFixture : public ::testing::Test {

public:
    DaemonHouseKeepingFixture() = default;

    void SetUp() override {
        HouseKeeping::InitFlags();
    }

    void TearDown() override {   
    }
};

TEST_F(DaemonHouseKeepingFixture, InitSignals_OnRegisterSigactionFailure_Fails) {   
    SignalMock mock_signal;
    g_mock_signal = &mock_signal;
    EXPECT_CALL(mock_signal, signalMock(_,  _)).Times(2).WillRepeatedly(Return(1));
    std::unique_ptr<HouseKeeping> house_keeping = std::make_unique<HouseKeeping>();
    house_keeping->InitSignals();
}

TEST_F(DaemonHouseKeepingFixture, InitSignals_OnRegisterSigactionSuccess_Succeeds) {   
    SignalMock mock_signal;
    g_mock_signal = &mock_signal;

    EXPECT_CALL(mock_signal, signalMock(_, _)).Times(2).WillRepeatedly(Return(0));

    std::unique_ptr<HouseKeeping> house_keeping = std::make_unique<HouseKeeping>();
    house_keeping->InitSignals();
}

TEST_F(DaemonHouseKeepingFixture, SignalHandler_HandleMultiSignals_Succeeds) {   
    SignalMock mock_signal;
    g_mock_signal = &mock_signal;

    EXPECT_CALL(mock_signal, raiseMock(_)).Times(1).WillRepeatedly(Return(0));
    SignalHandler(SIGINT);
    SignalHandler(SIGTERM);
    // for coverage of the empty else block
    SignalHandler(SIGABRT);
}

}  // namespace daemon_housekeeping_ut
}  // namespace testing
