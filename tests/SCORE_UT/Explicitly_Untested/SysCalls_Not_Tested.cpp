/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <fcntl.h> /* For O_* constants */
#include <stdarg.h>
#include <unistd.h>

#include "score/time/utility/SysCalls.h"

namespace score {
namespace time {

/// @brief Reasons for excluding SysCalls from testing
///
/// @defgroup SysCalls_Not_Tested SysCalls Not Tested
/// @verbatim embed:rst:leading-slashes
///
///  System call wrapper functions are used for mocking in unit tests, it does not make sense to write unit tests for
///  them. The reason is that unit tests are typically used to test the behavior of a specific unit of code in isolation
///  from the rest of the system. In the case of system call wrapper functions, the behavior of the function is largely
///  dependent on the behavior of the underlying system call. As a result, it may not be possible to fully test the
///  behavior of the wrapper function in isolation, without also testing the behavior of the system call it wraps.
///
/// @endverbatim

int OsShmOpen(const char* name, int oflag, mode_t mode) {
    return shm_open(name, oflag, mode);
}

int OsShmUnlink(const char* name) {
    return shm_unlink(name);
}

int OsFtruncate(int fd, off_t length) {
    return ftruncate(fd, length);
}

void* OsMmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset) {
    return mmap(addr, len, prot, flags, fd, offset);
}

int OsMunmap(void* addr, size_t len) {
    return munmap(addr, len);
}

int OsClose(int fd) {
    return close(fd);
}

sem_t* OsSemOpen(const char* name, int oflag, ...) {
    if ((oflag & O_CREAT) != 0) {
        // 2 additional parameters are required
        va_list argp;
        va_start(argp, oflag);
        mode_t mode = va_arg(argp, mode_t);
        unsigned value = va_arg(argp, unsigned int);
        va_end(argp);
        return sem_open(name, oflag, mode, value);
    } else {
        return sem_open(name, oflag);
    }
}

int OsSemClose(sem_t* sem) {
    return sem_close(sem);
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

}  // namespace time
}  // namespace score
