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
#pragma once

#include <cassert>
#include <cstdlib>
#include <pthread.h>

namespace score_time::utils
{

    /**
     * @brief RAII wrapper for pthread_mutex_t
     *
     * Automatically locks the mutex on construction and unlocks on destruction.
     * This prevents deadlocks from forgotten unlocks or early returns.
     *
     * @note This class is not copyable or movable
     * @warning Does not check pthread_mutex_lock/unlock return values.
     *          Assumes the mutex is properly initialized and not in an error state.
     *          For safety-critical systems requiring full AUTOSAR compliance,
     *          consider adding explicit error checks if required by your project rules.
     *
     * Example usage:
     * @code
     * pthread_mutex_t mtx;
     * pthread_mutex_init(&mtx, nullptr);
     * {
     *     PthreadLockGuard lock(&mtx);
     *     // Critical section - mutex is locked
     *     doSomething();
     * } // Mutex automatically unlocked here
     * pthread_mutex_destroy(&mtx);
     * @endcode
     */
    class PthreadLockGuard final
    {
    public:
        /**
         * @brief Construct and lock the mutex
         * @param mtx Pointer to pthread_mutex_t (must not be null)
         */
        explicit PthreadLockGuard(pthread_mutex_t *mtx) noexcept : mtx_(mtx)
        {
            assert(mtx_ && "PthreadLockGuard: mutex pointer must not be null");
            const int ret = ::pthread_mutex_lock(mtx_);
            // In safety-critical systems, lock failure is unrecoverable
            if (ret != 0)
            {
                std::abort();  // Terminate immediately - cannot proceed without lock
            }
        }

        /**
         * @brief Destruct and unlock the mutex
         */
        ~PthreadLockGuard() noexcept
        {
            assert(mtx_ && "PthreadLockGuard: mutex pointer must not be null");
            const int ret = ::pthread_mutex_unlock(mtx_);
            // In safety-critical systems, unlock failure is unrecoverable
            if (ret != 0)
            {
                std::abort();  // Terminate immediately - mutex state corrupted
            }
        }

        // Delete copy and move operations
        PthreadLockGuard(const PthreadLockGuard &) = delete;
        PthreadLockGuard &operator=(const PthreadLockGuard &) = delete;
        PthreadLockGuard(PthreadLockGuard &&) = delete;
        PthreadLockGuard &operator=(PthreadLockGuard &&) = delete;

    private:
        pthread_mutex_t *mtx_;
    };

} // namespace score_time::utils
