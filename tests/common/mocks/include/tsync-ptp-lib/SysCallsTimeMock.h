/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef TIMESYNC_PTP_LIB_SYSCALLSTIMEMOCK_H
#define TIMESYNC_PTP_LIB_SYSCALLSTIMEMOCK_H

#include <gmock/gmock.h>
#include <semaphore.h>

namespace score {
namespace time {

class SysCallsTimeMock {
   public:
    MOCK_METHOD2(OsClockGetTime, int(clockid_t clk_id, timespec* tp));

    ~SysCallsTimeMock() = default;
};

// Declare the mock object and initialize it in the test module
// extern std::unique_ptr<::testing::NiceMock<SysCallsTimeMock>> time_mock;
extern std::unique_ptr< testing::NiceMock< SysCallsTimeMock> > time_mock;
}  // namespace time
}  // namespace score

#endif  // TIMESYNC_PTP_LIB_SYSCALLSTIMEMOCK_H
