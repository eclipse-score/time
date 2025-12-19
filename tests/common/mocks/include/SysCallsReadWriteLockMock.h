/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYSCALLREADWRITELOCKMOCK_H_
#define SCORE_TIME_SYSCALLREADWRITELOCKMOCK_H_

#include <gmock/gmock.h>
#include <pthread.h>
#include <memory>

namespace score {
namespace time {

class SysCallsReadWriteLockMock {
public:
    SysCallsReadWriteLockMock() {
        ON_CALL(*this, OsRwLockInitAttr(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsRwLockDestroyAttr(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsRwLockAttrSetPShared(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsRwLockInit(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));

        ON_CALL(*this, OsRwLockTryReadLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsRwLockReadLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsRwLockTryWriteLock(::testing::_)).WillByDefault(::testing::Return(0));

        ON_CALL(*this, OsRwLockTryWriteLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsRwLockWriteLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsRwLockUnlock(::testing::_)).WillByDefault(::testing::Return(0));
    
        ON_CALL(*this, OsUmask(::testing::_)).WillByDefault(::testing::Return(0));
    }

    MOCK_METHOD1(OsRwLockInitAttr, int(pthread_rwlockattr_t*));
    MOCK_METHOD1(OsRwLockDestroyAttr, int(pthread_rwlockattr_t*));
    MOCK_METHOD2(OsRwLockAttrSetPShared, int(pthread_rwlockattr_t*, int));
    MOCK_METHOD2(OsRwLockInit, int(pthread_rwlock_t*, const pthread_rwlockattr_t*));
    MOCK_METHOD1(OsRwLockTryReadLock, int(pthread_rwlock_t*));
    MOCK_METHOD1(OsRwLockReadLock, int(pthread_rwlock_t*));
    MOCK_METHOD1(OsRwLockTryWriteLock, int(pthread_rwlock_t*));
    MOCK_METHOD1(OsRwLockWriteLock, int(pthread_rwlock_t*));
    MOCK_METHOD1(OsRwLockUnlock, int(pthread_rwlock_t*));
    MOCK_METHOD1(OsRwLockDestroyLock, int(pthread_rwlock_t*));
    MOCK_METHOD1(OsUmask, mode_t(mode_t));

    ~SysCallsReadWriteLockMock() = default;
};

// Declare the mock object and initialize it in the test module
extern std::unique_ptr<::testing::NiceMock<SysCallsReadWriteLockMock>> rw_lock_mock;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYSCALLREADWRITELOCKMOCK_H_
