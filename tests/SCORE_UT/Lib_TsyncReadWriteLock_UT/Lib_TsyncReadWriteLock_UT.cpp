/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>
#define private public
#include "score/time/utility/TsyncReadWriteLock.h"
#undef private
//!!#include "ara/core/abort.h"
#include "score/span.hpp"
#include "SysCallsReadWriteLockMock.h"

using namespace score::time;
std::unique_ptr<testing::NiceMock<SysCallsReadWriteLockMock>> score::time::rw_lock_mock;

class TSyncReadWriteLockTestFixture : public ::testing::Test {
   public:
    static const int32_t EXIT_CODE;

   protected:
    void SetUp() override {
        rw_lock_mock = std::make_unique<testing::NiceMock<SysCallsReadWriteLockMock>>();

        // Setup default expectations for success cases
        ON_CALL(*rw_lock_mock, OsRwLockInitAttr(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockDestroyAttr(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockAttrSetPShared(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockInit(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockTryReadLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockReadLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockTryWriteLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockWriteLock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockUnlock(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockDestroyLock(::testing::_)).WillByDefault(::testing::Return(0));

        // install abort handler for our death tests
        //!! ara::core::SetAbortHandler(&AbortHandler);
        // As we use here singleton mock object, clear expectations after each test
        ::testing::Mock::AllowLeak(rw_lock_mock.get());
    }

    void TearDown() override {
        rw_lock_mock.reset();

        //!! ara::core::SetAbortHandler(nullptr);
    }

    static void AbortHandler() noexcept {
        // the mock has to be reset here, otherwise the expectations for our death tests
        // will never be met/evaluated.
        rw_lock_mock.reset();
        std::exit(EXIT_CODE);
    }
};

const int32_t TSyncReadWriteLockTestFixture::EXIT_CODE = 42;

namespace testing {
namespace lib_tsyncreadwritelock_ut {

TEST_F(TSyncReadWriteLockTestFixture, MoveConstruction_Success) {
    // Act
    pthread_rwlock_t attr;
    TsyncReadWriteLock lock;
    lock.Open(&attr, TsyncReadWriteLock::LockMode::Read, true);
    auto ownership = lock.is_owner_;
    auto ptr = lock.rw_lock_;
    TsyncReadWriteLock lock2(std::move(lock));

    ASSERT_EQ(ownership, lock2.is_owner_);
    ASSERT_EQ(ptr, lock2.rw_lock_);
    ASSERT_EQ(lock.rw_lock_, nullptr);
}

TEST_F(TSyncReadWriteLockTestFixture, MoveAssignment_Success) {
    // Act
    pthread_rwlock_t attr;
    TsyncReadWriteLock lock;
    lock.Open(&attr, TsyncReadWriteLock::LockMode::Read, true);
    auto ownership = lock.is_owner_;
    auto ptr = lock.rw_lock_;
    TsyncReadWriteLock lock2;
    lock2 = std::move(lock);

    ASSERT_EQ(ownership, lock2.is_owner_);
    ASSERT_EQ(ptr, lock2.rw_lock_);
    ASSERT_EQ(lock.rw_lock_, nullptr);
}

TEST_F(TSyncReadWriteLockTestFixture, Open_OsRwLockInitAttrFail_Exit) {
    // Assert
    ASSERT_EXIT(
        {
            // Arange
            EXPECT_CALL(*rw_lock_mock, OsRwLockInitAttr(::testing::_)).WillOnce(::testing::Return(-1));

            // Act
            pthread_rwlock_t attr;
            TsyncReadWriteLock lock;
            lock.Open(&attr, TsyncReadWriteLock::LockMode::Read, true);
        },
        ::testing::ExitedWithCode(TSyncReadWriteLockTestFixture::EXIT_CODE), "pthreadrw_lock_attr_init failed");
}

TEST_F(TSyncReadWriteLockTestFixture, Open_OsRwLockAttrSetPSharedFail_Exit) {
    // Assert
    ASSERT_EXIT(
        {
            // Arange
            EXPECT_CALL(*rw_lock_mock, OsRwLockAttrSetPShared(::testing::_, ::testing::_))
                .WillOnce(::testing::Return(-1));

            // Act
            pthread_rwlock_t attr;
            TsyncReadWriteLock lock;
            lock.Open(&attr, TsyncReadWriteLock::LockMode::Read, true);
        },
        ::testing::ExitedWithCode(TSyncReadWriteLockTestFixture::EXIT_CODE), "pthreadrw_lock_attr_setpshared failed");
}

TEST_F(TSyncReadWriteLockTestFixture, Open_OsRwLockInitFail_Exit) {
    // Assert
    ASSERT_EXIT(
        {
            // Arange
            EXPECT_CALL(*rw_lock_mock, OsRwLockInit(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

            // Act
            pthread_rwlock_t attr;
            TsyncReadWriteLock lock;
            lock.Open(&attr, TsyncReadWriteLock::LockMode::Write, true);
        },
        ::testing::ExitedWithCode(TSyncReadWriteLockTestFixture::EXIT_CODE), "pthreadrw_lock_init failed");
}

TEST_F(TSyncReadWriteLockTestFixture, Open_ForNonOwnedReadLock_Success) {
    // Act
    pthread_rwlock_t attr;
    TsyncReadWriteLock lock;
    lock.Open(&attr, TsyncReadWriteLock::LockMode::Read, false);
    // Assert
    SUCCEED();
}

TEST_F(TSyncReadWriteLockTestFixture, try_lock_OsRwLockTryWriteLockSuccess_Success) {
    // Act
    pthread_rwlock_t attr;
    TsyncReadWriteLock lock;
    lock.Open(&attr, TsyncReadWriteLock::LockMode::Write, true);
    bool res = lock.try_lock();
    lock.unlock();
    // Assert
    ASSERT_TRUE(res);
}

TEST_F(TSyncReadWriteLockTestFixture, lock_OsRwLockWriteLockFail_Exit) {
    // Assert
    ASSERT_EXIT(
        {
            // Arange
            EXPECT_CALL(*rw_lock_mock, OsRwLockWriteLock(::testing::_)).WillOnce(::testing::Return(-1));

            // Act
            pthread_rwlock_t attr;
            TsyncReadWriteLock lock;
            lock.Open(&attr, TsyncReadWriteLock::LockMode::Write, true);

            lock.lock();
        },
        ::testing::ExitedWithCode(TSyncReadWriteLockTestFixture::EXIT_CODE), "TsyncReadWriteLock::lock\\(\\) failed");
}

TEST_F(TSyncReadWriteLockTestFixture, unlock_OsRwLockUnlockFail_Exit) {
    // Assert
    ASSERT_EXIT(
        {
            // Arange
            EXPECT_CALL(*rw_lock_mock, OsRwLockUnlock(::testing::_)).WillOnce(::testing::Return(-1));

            // Act
            pthread_rwlock_t attr;
            TsyncReadWriteLock lock;
            lock.Open(&attr, TsyncReadWriteLock::LockMode::Write, true);

            lock.lock();
            lock.unlock();
        },
        ::testing::ExitedWithCode(TSyncReadWriteLockTestFixture::EXIT_CODE), "TsyncReadWriteLock::unlock\\(\\) failed");
}

}  // namespace lib_tsyncreadwritelock_ut
}  // namespace testing
