/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYSCALLSNAMEDSEMMOCK_H_
#define SCORE_TIME_SYSCALLSNAMEDSEMMOCK_H_

#include <gmock/gmock.h>
#include <semaphore.h>

#include <memory>

namespace score {
namespace time {

class SysCallsNamedSemMock {
public:
    MOCK_METHOD2(OsSemOpen, sem_t*(const char*, int));
    MOCK_METHOD4(OsSemOpen, sem_t*(const char*, int, mode_t, unsigned int));
    MOCK_METHOD1(OsSemClose, int(sem_t*));
    MOCK_METHOD1(OsSemUnlink, int(const char*));
    MOCK_METHOD1(OsSemWait, int(sem_t*));
    MOCK_METHOD1(OsSemPost, int(sem_t*));
    MOCK_METHOD1(OsSemTryWait, int(sem_t*));
    MOCK_METHOD2(OsSemGetValue, int(sem_t*, int*));

    ~SysCallsNamedSemMock() = default;
};

// Declare the mock object and initialize it in the test module
extern std::unique_ptr<::testing::NiceMock<SysCallsNamedSemMock>> named_semaphore_mock;
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYSCALLSNAMEDSEMMOCK_H_
