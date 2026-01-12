/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "SysCallsNamedSemMock.h"

#include <fcntl.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <unistd.h>

namespace score {
namespace time {

std::unique_ptr<::testing::NiceMock<SysCallsNamedSemMock>> named_semaphore_mock;

sem_t* OsSemOpen(const char* name, int oflag) {
    return named_semaphore_mock->OsSemOpen(name, oflag);
}

sem_t* OsSemOpen(const char* name, int oflag, mode_t mode, unsigned int value) {
    return named_semaphore_mock->OsSemOpen(name, oflag, mode, value);
}

int OsSemClose(sem_t* sem) {
    return named_semaphore_mock->OsSemClose(sem);
}

int OsSemUnlink(const char* name) {
    return named_semaphore_mock->OsSemUnlink(name);
}

int OsSemWait(sem_t* sem) {
    return named_semaphore_mock->OsSemWait(sem);
}

int OsSemPost(sem_t* sem) {
    return named_semaphore_mock->OsSemPost(sem);
}

int OsSemTryWait(sem_t* sem) {
    return named_semaphore_mock->OsSemTryWait(sem);
}

int OsSemGetValue(sem_t* sem, int* value) {
    return named_semaphore_mock->OsSemGetValue(sem, value);
}

}  // namespace time
}  // namespace score
