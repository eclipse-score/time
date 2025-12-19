/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

/// @brief SynchronizedTimeBaseStatus Unit/Binary Tests
///
/// @page SynchronizedTimeBaseStatusUnitBinaryTests
/// @{
/// @verbatim embed:rst:leading-slashes
///
/// .. test_spec:: SynchronizedTimeBaseStatusTest
///     :id: TS_UT_TS_BT_SYNCTIMEBASESTATUS
///     :status: accepted
///     :satisfied_by: UT_BT_TESTFILE_SYNCTIMEBASESTATUS
///     :tests: SWS_TS_01052, SWS_TS_01053, SWS_TS_01054, SWS_TS_01055, SWS_TS_01056, SWS_TS_01057, SWS_TS_01058,
///             SWS_TS_01059, SWS_TS_01060, PUB_TSYNC_TIMEBASESTATUS_INTERFACE, SB_TSYNC_LIB
///     :tags: test_req_based, test_interface
///
///     **Test Scope:**
///
///         Verify the existence of the SynchronizedTimeBaseStatus class and the behavior of its methods.
///
///     **Test Design:**
///
///         Create an instance of the SynchronizedTimeBaseStatus class, and validate its APIs by using
///         the GoogleTest Framework.
///
///     **Test Cases:**
///
///         .. doxygengroup:: SyncTimeBaseStatus_Test_Cases
///            :project: tsync
///
/// @endverbatim
/// @}

#include <algorithm>
#include <gtest/gtest.h>

// class under test
#include "score/time/synchronized_time_base_status.h"
#include "TimeBaseStatusAccessMediator.h"

using score::time::LeapJump;
using score::time::SynchronizationStatus;
using score::time::SynchronizedTimeBaseStatus;
using score::time::Timestamp;
using score::time::TimeBaseStatusAccessMediator;

const SynchronizationStatus sync_status = SynchronizationStatus::kSynchronized;
const LeapJump leap_jump = LeapJump::kTimeLeapFuture;
const std::byte ud_arr[3] = {1, 2, 3};
const score::cpp::span<const std::byte> ud_span = ud_arr;

namespace testing {
namespace lib_synchronizedtimebasestatus_ut_bt {

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the existence of the SynchronizedTimeBaseStatus class via default construction.
///
/// **Tests:** :need:`SWS_TS_01052`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, ExistenceDefaultCtorDtor) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();

    ASSERT_EQ(status.GetSynchronizationStatus(), SynchronizationStatus::kNotSynchronizedUntilStartup);
    ASSERT_EQ(status.GetLeapJump(), LeapJump::kTimeLeapNone);
    ASSERT_EQ(status.GetCreationTime().time_since_epoch(), Timestamp::duration::zero());
    ASSERT_TRUE(status.GetUserData().empty());
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the SynchronizedTimeBaseStatus class move constructor.
///
/// **Tests:** :need:`SWS_TS_01057`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, MoveCtor) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();

    Timestamp time_stamp(std::chrono::system_clock::now().time_since_epoch());

    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusSynchronizationStatus(status, sync_status);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusLeapJump(status, leap_jump);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusCreationTime(status, time_stamp);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusUserData(status, ud_span);

    SynchronizedTimeBaseStatus moved_status(std::move(status));
    ASSERT_EQ(moved_status.GetSynchronizationStatus(), sync_status);
    ASSERT_EQ(moved_status.GetLeapJump(), leap_jump);
    ASSERT_EQ(moved_status.GetCreationTime(), time_stamp);
    ASSERT_TRUE(std::equal(moved_status.GetUserData().begin(), moved_status.GetUserData().end(), ud_span.begin(),
                           ud_span.end()));
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the SynchronizedTimeBaseStatus class copy constructor.
///
/// **Tests:** :need:`SWS_TS_01058`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, CopyCtor) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();

    Timestamp time_stamp(std::chrono::system_clock::now().time_since_epoch());

    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusSynchronizationStatus(status, sync_status);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusLeapJump(status, leap_jump);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusCreationTime(status, time_stamp);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusUserData(status, ud_span);

    SynchronizedTimeBaseStatus copied_status(status);
    ASSERT_EQ(copied_status.GetSynchronizationStatus(), sync_status);
    ASSERT_EQ(copied_status.GetLeapJump(), leap_jump);
    ASSERT_EQ(copied_status.GetCreationTime(), time_stamp);
    ASSERT_TRUE(std::equal(copied_status.GetUserData().begin(), copied_status.GetUserData().end(), ud_span.begin(),
                           ud_span.end()));
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the SynchronizedTimeBaseStatus class move assignment operator.
///
/// **Tests:** :need:`SWS_TS_01059`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, MoveAssignment) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();

    Timestamp time_stamp(std::chrono::system_clock::now().time_since_epoch());

    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusSynchronizationStatus(status, sync_status);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusLeapJump(status, leap_jump);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusCreationTime(status, time_stamp);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusUserData(status, ud_span);

    SynchronizedTimeBaseStatus moved_status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();
    moved_status = std::move(status);
    ASSERT_EQ(moved_status.GetSynchronizationStatus(), sync_status);
    ASSERT_EQ(moved_status.GetLeapJump(), leap_jump);
    ASSERT_EQ(moved_status.GetCreationTime(), time_stamp);
    ASSERT_TRUE(std::equal(moved_status.GetUserData().begin(), moved_status.GetUserData().end(), ud_span.begin(),
                           ud_span.end()));
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the SynchronizedTimeBaseStatus class copy assignment operator.
///
/// **Tests:** :need:`SWS_TS_01060`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, CopyAssignment) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();

    Timestamp time_stamp(std::chrono::system_clock::now().time_since_epoch());

    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusSynchronizationStatus(status, sync_status);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusLeapJump(status, leap_jump);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusCreationTime(status, time_stamp);
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusUserData(status, ud_span);

    SynchronizedTimeBaseStatus copied_status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();
    copied_status = status;

    ASSERT_EQ(copied_status.GetSynchronizationStatus(), sync_status);
    ASSERT_EQ(copied_status.GetLeapJump(), leap_jump);
    ASSERT_EQ(copied_status.GetCreationTime(), time_stamp);
    ASSERT_TRUE(std::equal(copied_status.GetUserData().begin(), copied_status.GetUserData().end(), ud_span.begin(),
                           ud_span.end()));
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the GetSynchronizationStatus() API.
///
/// **Tests:** :need:`SWS_TS_01053`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, GetSynchronizationStatus) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusSynchronizationStatus(status, sync_status);

    ASSERT_EQ(status.GetSynchronizationStatus(), sync_status);
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the GetLeapJump() API.
///
/// **Tests:** :need:`SWS_TS_01054`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, GetLeapJump) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusLeapJump(status, leap_jump);

    ASSERT_EQ(status.GetLeapJump(), leap_jump);
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the GetCreationTime() API.
///
/// **Tests:** :need:`SWS_TS_01055`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, GetCreationTime) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();
    Timestamp time_stamp(std::chrono::system_clock::now().time_since_epoch());
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusCreationTime(status, time_stamp);

    ASSERT_EQ(status.GetCreationTime(), time_stamp);
}

/// @addtogroup SyncTimeBaseStatus_Test_Cases
/// @verbatim embed:rst:leading-slashes
/// **Test Case:** Verify the behavior of the GetUserData() API.
///
/// **Tests:** :need:`SWS_TS_01056`
///
/// **Expected Result:** All assertions hold.
///
/// @endverbatim
TEST(SynchronizedTimeBaseStatusTest, GetUserData) {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::GetSynchronizedTimeBaseStatusInstance();
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusUserData(status, ud_span);

    ASSERT_TRUE(std::equal(status.GetUserData().begin(), status.GetUserData().end(), ud_span.begin(), ud_span.end()));
}

}  // namespace lib_synchronizedtimebasestatus_ut_bt
}  // namespace testing
