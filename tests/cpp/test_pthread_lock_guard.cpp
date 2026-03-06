/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/
#include <gtest/gtest.h>
#include "score_time/utils/pthread_lock_guard.hpp"

#include <atomic>
#include <pthread.h>
#include <thread>
#include <type_traits>

namespace {

using score_time::utils::PthreadLockGuard;

// ─── RAII Locking Behaviour ──────────────────────────────────

TEST(PthreadLockGuardTest, LockOnConstruction_MutexIsLocked)
{
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    {
        PthreadLockGuard guard(&mtx);

        // Verify that another thread cannot acquire the mutex (trylock fails)
        std::atomic<bool> trylock_succeeded{false};
        std::thread t([&] {
            trylock_succeeded.store(pthread_mutex_trylock(&mtx) == 0,
                                    std::memory_order_release);
        });
        t.join();
        EXPECT_FALSE(trylock_succeeded.load(std::memory_order_acquire));
    }  // guard destroyed here — mutex should be released

    // Now the mutex must be acquirable again
    EXPECT_EQ(pthread_mutex_trylock(&mtx), 0);
    pthread_mutex_unlock(&mtx);
    pthread_mutex_destroy(&mtx);
}

TEST(PthreadLockGuardTest, UnlockOnDestruction_MutexIsUnlocked)
{
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    {
        PthreadLockGuard guard(&mtx);
        (void)guard;
    }
    // Mutex must be free after guard leaves scope
    EXPECT_EQ(pthread_mutex_trylock(&mtx), 0);
    pthread_mutex_unlock(&mtx);
    pthread_mutex_destroy(&mtx);
}

TEST(PthreadLockGuardTest, MultipleScopes_NoDeadlock)
{
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    for (int i = 0; i < 10; ++i)
    {
        PthreadLockGuard guard(&mtx);
        (void)guard;
    }
    pthread_mutex_destroy(&mtx);
}

TEST(PthreadLockGuardTest, NestedScopes_MutexReleasedAtEachScope)
{
    // Each scope should fully lock and unlock independently
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    {
        PthreadLockGuard g1(&mtx);
        (void)g1;
        // mutex locked
    }
    // mutex unlocked — can lock again
    {
        PthreadLockGuard g2(&mtx);
        (void)g2;
    }
    pthread_mutex_destroy(&mtx);
}

// ─── Type Traits ─────────────────────────────────────────────

TEST(PthreadLockGuardTest, NotCopyConstructible)
{
    EXPECT_FALSE(std::is_copy_constructible<PthreadLockGuard>::value);
}

TEST(PthreadLockGuardTest, NotMoveConstructible)
{
    EXPECT_FALSE(std::is_move_constructible<PthreadLockGuard>::value);
}

TEST(PthreadLockGuardTest, NotCopyAssignable)
{
    EXPECT_FALSE(std::is_copy_assignable<PthreadLockGuard>::value);
}

TEST(PthreadLockGuardTest, NotMoveAssignable)
{
    EXPECT_FALSE(std::is_move_assignable<PthreadLockGuard>::value);
}

}  // namespace
