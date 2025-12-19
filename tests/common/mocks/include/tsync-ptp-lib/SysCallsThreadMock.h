/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef TIMESYNC_PTP_LIB_SYSCALLSTHREADMOCK_H
#define TIMESYNC_PTP_LIB_SYSCALLSTHREADMOCK_H

#include <gmock/gmock.h>
#include <semaphore.h>

namespace score {
namespace time {

class SysCallsThreadMock {
   public:
    MOCK_METHOD4(OsThreadCreate,
                 int(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg));

    MOCK_METHOD2(OsThreadJoin, int(pthread_t thread, void** retval));

    ~SysCallsThreadMock() = default;
};

// Declare the mock object and initialize it in the test module
// extern std::unique_ptr<::testing::NiceMock<SysCallsThreadMock>> thread_mock;
extern std::unique_ptr< testing::NiceMock< SysCallsThreadMock> > thread_mock;

}  // namespace time
}  // namespace score

#endif  // TIMESYNC_PTP_LIB_SYSCALLSTHREADMOCK_H
