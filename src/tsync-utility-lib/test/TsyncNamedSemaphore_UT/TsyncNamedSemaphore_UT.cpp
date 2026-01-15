/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csignal>
#include <cstddef>

#define private public
#include "score/time/utility/TsyncNamedSemaphore.h"
#undef private
#include "SysCallsNamedSemMock.h"

using namespace score::time;

#include <iostream>

class TSyncNamedSemaphoreTestFixture : public ::testing::Test {
public:
    static const int32_t EXIT_CODE;
    static const char* TEST_LOCK_NAME;
    static const char* TEST_LOCK_NAME_2;

protected:
    void SetUp() override {
        named_semaphore_mock = std::make_unique<::testing::NiceMock<SysCallsNamedSemMock>>();

        // Setup default expectations
        // unsignaled open
        ON_CALL(*named_semaphore_mock, OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, 0))
            .WillByDefault(::testing::DoAll([this]() { this->sem_val_ = 0; }, ::testing::Return(&dummy_sem_)));
        ON_CALL(*named_semaphore_mock, OsSemOpen(::testing::StrEq(TEST_LOCK_NAME_2), ::testing::_, ::testing::_, 0))
            .WillByDefault(::testing::DoAll([this]() { this->sem_val_2_ = 0; }, ::testing::Return(&dummy_sem_2_)));
        // signaled open
        ON_CALL(*named_semaphore_mock, OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, 1))
            .WillByDefault(::testing::DoAll([this]() { this->sem_val_ = 1; }, ::testing::Return(&dummy_sem_)));
        ON_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*named_semaphore_mock, OsSemUnlink(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*named_semaphore_mock, OsSemWait(::testing::_))
            .WillByDefault(::testing::DoAll(
                [this]() {
                    if (this->sem_val_ > 0) {
                        --this->sem_val_;
                    }
                },
                ::testing::Return(0)));
        ON_CALL(*named_semaphore_mock, OsSemPost(::testing::_))
            .WillByDefault(::testing::DoAll([this]() { ++this->sem_val_; }, ::testing::Return(0)));
        ON_CALL(*named_semaphore_mock, OsSemTryWait(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*named_semaphore_mock, OsSemGetValue(::testing::_, ::testing::_))
            .WillByDefault(::testing::DoAll(testing::SetArgPointee<1>(::testing::ByRef(sem_val_)), testing::Return(0)));

        ::testing::Mock::AllowLeak(named_semaphore_mock.get());
        std::signal(SIGABRT, AbortHandler);
    }

    void TearDown() override {
        // As we use here singleton mock object, clear expectations after each test
        named_semaphore_mock.reset(nullptr);
        std::signal(SIGABRT, SIG_DFL);
    }

    static void AbortHandler(int /*signal*/) noexcept {
        // the mock has to be reset here, otherwise the expectations for our death tests
        // will never be met/evaluated.
        named_semaphore_mock.reset(nullptr);
        std::exit(TSyncNamedSemaphoreTestFixture::EXIT_CODE);
    }

    sem_t dummy_sem_;
    sem_t dummy_sem_2_;
    unsigned int sem_val_;
    unsigned int sem_val_2_;
};

const int32_t TSyncNamedSemaphoreTestFixture::EXIT_CODE = 42;
const char* TSyncNamedSemaphoreTestFixture::TEST_LOCK_NAME = "/TsyncNamedSemaphore_test";
const char* TSyncNamedSemaphoreTestFixture::TEST_LOCK_NAME_2 = "/TsyncNamedSemaphore_test_2";

namespace testing {
namespace lib_tsyncnamedsemaphore_ut {

TEST_F(TSyncNamedSemaphoreTestFixture, MoveAssignmentOperator_Succeeds) {
    TsyncNamedSemaphore source(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    TsyncNamedSemaphore dest(TEST_LOCK_NAME_2, TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    EXPECT_STREQ(dest.name_.c_str(), TEST_LOCK_NAME_2);
    EXPECT_EQ(dest.is_owner_, true);
    dest = std::move(source);
    EXPECT_STREQ(dest.name_.c_str(), TEST_LOCK_NAME);
    EXPECT_EQ(dest.is_owner_, true);
}

TEST_F(TSyncNamedSemaphoreTestFixture, TryLock_InSignaledState_Succeeds) {
    // arrange
    EXPECT_CALL(*named_semaphore_mock,
                OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, ::testing::Eq(1)))
        .Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemTryWait(::testing::_)).Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemUnlink(::testing::StrEq(TEST_LOCK_NAME))).Times(1);

    TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Signaled, true);

    // act
    auto result = lock.try_lock();

    // assert
    ASSERT_TRUE(result);
}

TEST_F(TSyncNamedSemaphoreTestFixture, TryLock_InUnsignaledState_Fails) {
    // arrange
    EXPECT_CALL(*named_semaphore_mock,
                OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, ::testing::Eq(0)))
        .Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemTryWait(::testing::_)).Times(1).WillOnce(::testing::Return(-1));
    EXPECT_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemUnlink(::testing::StrCaseEq(TEST_LOCK_NAME))).Times(1);

    TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled, true);

    // act
    auto result = lock.try_lock();

    // assert
    EXPECT_FALSE(result);
}

TEST_F(TSyncNamedSemaphoreTestFixture, Ctor_OnSemOpenFailure_Aborts) {
    // arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*named_semaphore_mock, OsSemOpen(::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .Times(1)
                .WillOnce(::testing::Return(SEM_FAILED));

            // act
            TsyncNamedSemaphore sem(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Signaled);
        },
        ::testing::ExitedWithCode(TSyncNamedSemaphoreTestFixture::EXIT_CODE), ".*failed to create semaphore.*");
}

TEST_F(TSyncNamedSemaphoreTestFixture, GetValue_OnError_ReturnsMinusOne) {
    // arrange
    EXPECT_CALL(*named_semaphore_mock,
                OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, ::testing::Eq(1)))
        .Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemGetValue(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(-1));
    EXPECT_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemUnlink(::testing::StrEq(TEST_LOCK_NAME))).Times(1);

    TsyncNamedSemaphore sem(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Signaled, true);

    // act
    auto result = sem.get_value();

    // assert
    ASSERT_EQ(result, -1);
}

TEST_F(TSyncNamedSemaphoreTestFixture, GetValue_WithVariousSemaphoreStates_ReturnsCorrectValues) {
    // arrange
    EXPECT_CALL(*named_semaphore_mock,
                OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, ::testing::Eq(0)))
        .Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemWait(::testing::_)).Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemPost(::testing::_)).Times(3);
    EXPECT_CALL(*named_semaphore_mock, OsSemGetValue(::testing::_, ::testing::_)).Times(5);
    EXPECT_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemUnlink(::testing::StrEq(TEST_LOCK_NAME))).Times(1);

    TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled, true);

    // act and assert (various steps)
    int v = lock.get_value();
    ASSERT_EQ(0, v);
    lock.unlock();
    v = lock.get_value();
    ASSERT_EQ(1, v);
    lock.unlock();
    ASSERT_EQ(2, lock.get_value());
    lock.unlock();
    ASSERT_EQ(3, lock.get_value());
    lock.lock();
    ASSERT_EQ(2, lock.get_value());
}

TEST_F(TSyncNamedSemaphoreTestFixture, TryLock_WithUnsignaledOwnedSemapore_Fails) {
    // arrange
    EXPECT_CALL(*named_semaphore_mock,
                OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, ::testing::Eq(0)))
        .Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemTryWait(::testing::_)).Times(1).WillOnce(::testing::Return(-1));
    EXPECT_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemUnlink(::testing::StrEq(TEST_LOCK_NAME))).Times(1);

    TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled, true);

    // act
    auto result = lock.try_lock();

    // assert
    ASSERT_FALSE(result);
}

TEST_F(TSyncNamedSemaphoreTestFixture, TryLock_WithUnsignaledSemapore_Fails) {
    // arrange
    EXPECT_CALL(*named_semaphore_mock,
                OsSemOpen(::testing::StrEq(TEST_LOCK_NAME), ::testing::_, ::testing::_, ::testing::Eq(0)))
        .Times(1);
    EXPECT_CALL(*named_semaphore_mock, OsSemTryWait(::testing::_)).Times(1).WillOnce(::testing::Return(-1));
    EXPECT_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).Times(1);

    TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled, false);

    // act
    auto result = lock.try_lock();

    // assert
    ASSERT_FALSE(result);
}

TEST_F(TSyncNamedSemaphoreTestFixture, Dtor_OnSemCloseError_Aborts) {
    // arrange
    ASSERT_EXIT(
        {
            std::unique_ptr<TsyncNamedSemaphore> sem =
                std::make_unique<TsyncNamedSemaphore>(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled, true);
            EXPECT_CALL(*named_semaphore_mock, OsSemClose(::testing::_)).Times(1).WillOnce(::testing::Return(-1));
            sem.reset();
        },
        ::testing::ExitedWithCode(TSyncNamedSemaphoreTestFixture::EXIT_CODE), ".*sem_close failed for semaphore.*");
}

TEST_F(TSyncNamedSemaphoreTestFixture, Dtor_OnSemUnlinkError_Aborts) {
    // arrange
    ASSERT_EXIT(
        {
            std::unique_ptr<TsyncNamedSemaphore> sem =
                std::make_unique<TsyncNamedSemaphore>(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled, true);
            EXPECT_CALL(*named_semaphore_mock, OsSemUnlink(::testing::_)).Times(1).WillOnce(::testing::Return(-1));
            sem.reset();
        },
        ::testing::ExitedWithCode(TSyncNamedSemaphoreTestFixture::EXIT_CODE), ".*sem_unlink failed for semaphore.*");
}

TEST_F(TSyncNamedSemaphoreTestFixture, Lock_OnError_Aborts) {
    // arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*named_semaphore_mock, OsSemWait(::testing::_)).Times(1).WillOnce(::testing::Return(-1));

            TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled);

            // act
            lock.lock();
        },
        ::testing::ExitedWithCode(TSyncNamedSemaphoreTestFixture::EXIT_CODE), ".*sem_wait failed for semaphore.*");
}

TEST_F(TSyncNamedSemaphoreTestFixture, Lock_OnInterrupt_Success) {
    // arrange
    EXPECT_CALL(*named_semaphore_mock, OsSemWait(::testing::_))
        .WillOnce([]() {
            errno = EINTR;
            return -1;
        })
        .WillOnce(Return(0));

    TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled);

    // act
    lock.lock();
}

TEST_F(TSyncNamedSemaphoreTestFixture, Unlock_OnError_Aborts) {
    // arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*named_semaphore_mock, OsSemPost(::testing::_)).Times(1).WillOnce(::testing::Return(-1));

            TsyncNamedSemaphore lock(TEST_LOCK_NAME, TsyncNamedSemaphore::OpenMode::Unsignaled);

            // act
            lock.unlock();
        },
        ::testing::ExitedWithCode(TSyncNamedSemaphoreTestFixture::EXIT_CODE), ".*sem_post failed for semaphore.*");
}

}  // namespace lib_tsyncnamedsemaphore_ut
}  // namespace testing
