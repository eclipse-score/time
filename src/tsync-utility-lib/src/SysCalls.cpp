/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/utility/SysCalls.h"

#include <fcntl.h> /* For O_* constants */
#include <unistd.h>

#include <cstdarg>

#include "score/time/common/ExcludeCoverageAdapter.h"

namespace score {
namespace time {

EXCLUDE_COVERAGE_START(
    "System call wrapper functions are used for mocking in unit tests, it does not make sense to write unit tests for them.")

int OsShmOpen(const char* name, int oflag, mode_t mode) {
    return shm_open(name, oflag, mode);
}

int OsShmUnlink(const char* name) {
    return shm_unlink(name);
}

int OsFtruncate(int fd, off_t length) {
    return ftruncate(fd, length);
}

// clang-format off
// RULECHECKER_comment(1, 1, check_max_parameters, "The number of arguments reflects the signature of the underlying POSIX API.", true)
void* OsMmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset) {
    // clang-format on
    return mmap(addr, len, prot, flags, fd, offset);
}

int OsMunmap(void* addr, size_t len) {
    return munmap(addr, len);
}

int OsClose(int fd) {
    return close(fd);
}

sem_t* OsSemOpen(const char* name, int oflag) {
    return sem_open(name, oflag);
}

// clang-format off
// RULECHECKER_comment(1, 1, check_max_parameters, "The number of arguments reflects the signature of the underlying POSIX API.", true)
sem_t* OsSemOpen(const char* name, int oflag, mode_t mode, unsigned int value) {
    // clang-format on
    return sem_open(name, oflag, mode, value);
}

int OsSemClose(sem_t* sem) {
    return sem_close(sem);
}

mode_t OsUmask(mode_t mode) {
    return umask(mode);
}

int OsSemUnlink(const char* name) {
    return sem_unlink(name);
}

int OsSemWait(sem_t* sem) {
    return sem_wait(sem);
}

int OsSemPost(sem_t* sem) {
    return sem_post(sem);
}

int OsSemTryWait(sem_t* sem) {
    return sem_trywait(sem);
}

int OsSemGetValue(sem_t* sem, int* value) {
    return sem_getvalue(sem, value);
}

int OsRwLockInitAttr(pthread_rwlockattr_t* attr) {
    return pthread_rwlockattr_init(attr);
}

int OsRwLockDestroyAttr(pthread_rwlockattr_t* attr) {
    return pthread_rwlockattr_destroy(attr);
}

int OsRwLockAttrSetPShared(pthread_rwlockattr_t* attr, int shared) {
    return pthread_rwlockattr_setpshared(attr, shared);
}

int OsRwLockInit(pthread_rwlock_t* rw_lock, const pthread_rwlockattr_t* attr) {
    return pthread_rwlock_init(rw_lock, attr);
}

int OsRwLockTryReadLock(pthread_rwlock_t* rw_lock) {
    return pthread_rwlock_tryrdlock(rw_lock);
}

int OsRwLockReadLock(pthread_rwlock_t* rw_lock) {
    return pthread_rwlock_rdlock(rw_lock);
}

int OsRwLockTryWriteLock(pthread_rwlock_t* rw_lock) {
    return pthread_rwlock_trywrlock(rw_lock);
}

int OsRwLockWriteLock(pthread_rwlock_t* rw_lock) {
    return pthread_rwlock_wrlock(rw_lock);
}

int OsRwLockUnlock(pthread_rwlock_t* rw_lock) {
    return pthread_rwlock_unlock(rw_lock);
}

int OsRwLockDestroyLock(pthread_rwlock_t* rw_lock) {
    return pthread_rwlock_destroy(rw_lock);
}

int OsClockGetTime(clockid_t clk_id, timespec* tp) {
    return clock_gettime(clk_id, tp);
}

// clang-format off
// RULECHECKER_comment(1, 1, check_max_parameters, "The number of arguments reflects the signature of the underlying POSIX API.", true)
int OsThreadCreate(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    // clang-format on
    return pthread_create(thread, attr, start_routine, arg);
}

int OsThreadJoin(pthread_t thread, void** retval) {
    return pthread_join(thread, retval);
}

EXCLUDE_COVERAGE_END

}  // namespace time
}  // namespace score
