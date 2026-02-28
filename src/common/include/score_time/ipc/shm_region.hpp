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
#include <cstddef>
#include <string>

namespace score_time::ipc
{

    /**
     * @brief RAII wrapper for POSIX shared memory regions
     *
     * Manages lifecycle of shared memory objects (/dev/shm on Linux, shared memory
     * objects on QNX) with automatic cleanup. Uses shm_open + mmap for portable
     * POSIX shared memory access.
     *
     * **Features:**
     * - RAII: Automatically unmaps and closes on destruction
     * - Move-only: Prevents accidental copies of memory mappings
     * - Create-or-open semantics for both producer and consumer
     * - Size validation for safety
     *
     * **Typical Usage:**
     * - Producer (tsyncd): Open with create_or_open=true, writes SharedState
     * - Consumer (clients): Open with create_or_open=false, reads SharedState
     *
     * @note Not thread-safe: Each thread should have its own ShmRegion instance
     * @note Shared memory persists until explicitly unlinked or system reboot
     */
    class ShmRegion final
    {
    public:
        ShmRegion() = default;
        ~ShmRegion();

        ShmRegion(const ShmRegion &) = delete;
        ShmRegion &operator=(const ShmRegion &) = delete;

        ShmRegion(ShmRegion &&other) noexcept;
        ShmRegion &operator=(ShmRegion &&other) noexcept;

        /**
         * @brief Open or create a shared memory region
         *
         * @param name Shared memory name (e.g., "/score_time")
         * @param size Size of region in bytes (must match SharedState size)
         * @param create_or_open If true, create if missing; if false, open existing only
         * @return true if successful, false on error
         *
         * @note If create_or_open=true and region exists, it will be resized to match size parameter
         * @note If create_or_open=false and region doesn't exist, returns false
         */
        bool Open(const std::string &name, std::size_t size, bool create_or_open);

        /**
         * @brief Close and unmap the shared memory region
         *
         * Called automatically by destructor. Safe to call multiple times.
         */
        void Close();

        void *Addr() const { return addr_; }           ///< Get mapped memory address
        std::size_t Size() const { return size_; }     ///< Get region size in bytes
        int Fd() const { return fd_; }                 ///< Get file descriptor (for debugging)
        bool Valid() const { return addr_ != nullptr; } ///< Check if region is open and mapped

    private:
        std::string name_;
        int fd_ = -1;
        void *addr_ = nullptr;
        std::size_t size_ = 0;
    };

}
