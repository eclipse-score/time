/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "SysCallsMiscMock.h"

namespace score {
namespace time {

std::unique_ptr<::testing::NiceMock<SysCallsMiscMock>> misc_mock;

mode_t OsUmask(mode_t mode) {
    return misc_mock->OsUmask(mode);
}

int OsClockGetTime(clockid_t clk_id, timespec* tp) {
    return misc_mock->OsClockGetTime(clk_id, tp);
}

int OsThreadCreate(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    return misc_mock->OsThreadCreate(thread, attr, start_routine, arg);
}

int OsThreadJoin(pthread_t thread, void** retval) {
    return misc_mock->OsThreadJoin(thread, retval);
}


}  // namespace time
}  // namespace score
