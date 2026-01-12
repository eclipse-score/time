/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "SysCallsMiscMock.h"
#include "SysCallsNamedSemMock.h"

#define private public
#include "score/time/utility/TsyncSharedUtils.h"
#undef private

namespace score {
namespace time {
std::unique_ptr<::testing::NiceMock<SysCallsMiscMock>> misc_mock;
std::unique_ptr<::testing::NiceMock<SysCallsNamedSemMock>> named_semaphore_mock;
}  // namespace time
}  // namespace score

using namespace score::time;

using testing::_;
using testing::Invoke;

class TsyncSharedUtilsTestFixture : public ::testing::Test {
public:
    void SetUp() override {
        tsync_util_ = std::make_unique<TsyncSharedUtils>();
        misc_mock = std::make_unique<::testing::NiceMock<SysCallsMiscMock>>();
        named_semaphore_mock = std::make_unique<::testing::NiceMock<SysCallsNamedSemMock>>();
    }

    void TearDown() override {
        tsync_util_.reset();
        misc_mock.reset();
        named_semaphore_mock.reset();
    }

protected:
    std::unique_ptr<TsyncSharedUtils> tsync_util_;
    sem_t dummy_semaphore;
};

namespace testing {
namespace lib_tsyncsharedutils_ut {

TEST_F(TsyncSharedUtilsTestFixture, GetTransmissionSemaphoreName_ReturnValue_Succeed) {
    std::uint16_t time_domain_id = 1;
    std::string res = tsync_util_->GetTransmissionSemaphoreName(time_domain_id);
    EXPECT_EQ(std::string("time_domain_1"), res);
}

TEST_F(TsyncSharedUtilsTestFixture, CreateTransmissionSemaphore_ReturnValidSemaphore_Succeed) {
    std::uint16_t time_domain_id = 1;
    std::string domain_name = tsync_util_->GetTransmissionSemaphoreName(time_domain_id);
    bool as_owner = false;
    EXPECT_CALL(*named_semaphore_mock, OsSemOpen(::testing::StrEq("/time_domain_1"), _, _, _))
        .WillOnce(Return(&dummy_semaphore));

    tsync_util_->CreateTransmissionSemaphore(1, as_owner);
}

TEST_F(TsyncSharedUtilsTestFixture, GetCurrentVirtualLocalTime_OsClockGetTime_Failed) {
    EXPECT_CALL(*misc_mock, OsClockGetTime(CLOCK_MONOTONIC, _)).WillOnce(Return(-1));

    auto res = tsync_util_->GetCurrentVirtualLocalTime();

    EXPECT_EQ(std::nullopt, res);
}

TEST_F(TsyncSharedUtilsTestFixture, GetCurrentVirtualLocalTime_OsClockGetTime_Succeed) {
    EXPECT_CALL(*misc_mock, OsClockGetTime(CLOCK_MONOTONIC, _)).WillOnce(Return(0));

    auto res = tsync_util_->GetCurrentVirtualLocalTime();

    EXPECT_NE(std::nullopt, res);
}

TEST_F(TsyncSharedUtilsTestFixture, GetCurrentVirtualLocalTime_OsClockGetTime_Succeed_Overflow) {
    timespec ts;
    ts.tv_nsec = 999999999;
    ts.tv_sec = std::numeric_limits<uint64_t>::max();  // almost Overflowing sec

    EXPECT_CALL(*misc_mock, OsClockGetTime(_, _)).WillOnce(DoAll(SetArgPointee<1>(ts), Return(0)));

    auto res = tsync_util_->GetCurrentVirtualLocalTime();

    EXPECT_NE(std::nullopt, res);
}

}  // namespace lib_tsyncsharedutils_ut
}  // namespace testing
