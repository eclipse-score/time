/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYSCALLSMISCMOCK_H_
#define SCORE_TIME_SYSCALLSMISCMOCK_H_

#include <gmock/gmock.h>
#include <memory>

namespace score {
namespace time {

class SysCallsMiscMock {
public:
    MOCK_METHOD1(OsUmask, mode_t(mode_t));
    MOCK_METHOD2(OsClockGetTime, int(clockid_t, timespec*));
    MOCK_METHOD4(OsThreadCreate,
                 int(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg));
    MOCK_METHOD2(OsThreadJoin, int(pthread_t thread, void** retval));

    ~SysCallsMiscMock() = default;
};

// Declare the mock object and initialize it in the test module
extern std::unique_ptr<::testing::NiceMock<SysCallsMiscMock>> misc_mock;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYSCALLSMISCMOCK_H_
