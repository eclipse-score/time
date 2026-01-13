/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "HouseKeeping.h"

using  score::time::daemon::HouseKeeping;

namespace testing {
namespace housekeeping_ut {

class HouseKeepingFixture : public ::testing::Test {
public:
    HouseKeepingFixture() {
        HouseKeeping::Init();
    }
};

TEST_F(HouseKeepingFixture, Init_HkNotInitialized_ExitFlagIsZero) {
    EXPECT_EQ(HouseKeeping::exit_flag_, 0);
}

TEST_F(HouseKeepingFixture, RaiseSigInt_HkInitialized_ExitFlagIsSet) {
    std::raise(SIGINT);
    EXPECT_EQ(HouseKeeping::exit_flag_, 1);
}

TEST_F(HouseKeepingFixture, raiseSigTerm_HkInitialized_ExitFlagIsSet) {
    std::raise(SIGTERM);
    EXPECT_EQ(HouseKeeping::exit_flag_, 1);
}

TEST_F(HouseKeepingFixture, raiseSigInt_AlreadyRaisedBefore_ExitFlagIsSet) {
    std::raise(SIGINT);
    std::raise(SIGINT);
    EXPECT_EQ(HouseKeeping::exit_flag_, 1);
}

TEST_F(HouseKeepingFixture, raiseSigTerm_AlreadyRaisedBefore_ExitFlagIsSet) {
    std::raise(SIGTERM);
    std::raise(SIGTERM);
    EXPECT_EQ(HouseKeeping::exit_flag_, 1);
}

TEST_F(HouseKeepingFixture, raiseSigInt_SigTermRaisedBefore_ExitFlagIsSet) {
    std::raise(SIGTERM);
    std::raise(SIGINT);
    EXPECT_EQ(HouseKeeping::exit_flag_, 1);
}

TEST_F(HouseKeepingFixture, raiseSigTerm_SigIntRaisedBefore_ExitFlagIsSet) {
    std::raise(SIGINT);
    std::raise(SIGTERM);
    EXPECT_EQ(HouseKeeping::exit_flag_, 1);
}

TEST_F(HouseKeepingFixture, raiseSigCont_HkInitialized_ExitFlagIsZero) {
    std::raise(SIGCONT);
    EXPECT_EQ(HouseKeeping::exit_flag_, 0);
}

}  // namespace housekeeping_ut
}  // namespace testing
