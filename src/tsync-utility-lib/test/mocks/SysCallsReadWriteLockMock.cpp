/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "SysCallsReadWriteLockMock.h"

namespace score {
namespace time {

std::unique_ptr<testing::NiceMock<SysCallsReadWriteLockMock>> rw_lock_mock;

int OsRwLockInitAttr(pthread_rwlockattr_t* attr) {
    return rw_lock_mock->OsRwLockInitAttr(attr);
}

int OsRwLockDestroyAttr(pthread_rwlockattr_t* attr) {
    return rw_lock_mock->OsRwLockDestroyAttr(attr);
}

int OsRwLockAttrSetPShared(pthread_rwlockattr_t* attr, int shared) {
    return rw_lock_mock->OsRwLockAttrSetPShared(attr, shared);
}

int OsRwLockInit(pthread_rwlock_t* rw_lock, const pthread_rwlockattr_t* attr) {
    return rw_lock_mock->OsRwLockInit(rw_lock, attr);
}

int OsRwLockTryReadLock(pthread_rwlock_t* rw_lock) {
    return rw_lock_mock->OsRwLockTryReadLock(rw_lock);
}

int OsRwLockReadLock(pthread_rwlock_t* rw_lock) {
    return rw_lock_mock->OsRwLockReadLock(rw_lock);
}

int OsRwLockTryWriteLock(pthread_rwlock_t* rw_lock) {
    return rw_lock_mock->OsRwLockTryWriteLock(rw_lock);
}

int OsRwLockWriteLock(pthread_rwlock_t* rw_lock) {
    return rw_lock_mock->OsRwLockWriteLock(rw_lock);
}

int OsRwLockUnlock(pthread_rwlock_t* rw_lock) {
    return rw_lock_mock->OsRwLockUnlock(rw_lock);
}

int OsRwLockDestroyLock(pthread_rwlock_t* rw_lock) {
    return rw_lock_mock->OsRwLockDestroyLock(rw_lock);
}

}  // namespace time
}  // namespace score
