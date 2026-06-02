/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#ifndef SCORE_TS_CLIENT_SRC_GPTP_IPC_TEST_UTILS_H
#define SCORE_TS_CLIENT_SRC_GPTP_IPC_TEST_UTILS_H

#include "score/ts_client/src/gptp_ipc_channel.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/// Generate a unique POSIX shm name per invocation (avoids cross-test pollution).
inline std::string UniqueShmName()
{
    static std::atomic<int> counter{0};
    return "/gptp_ipc_ut_" + std::to_string(::getpid()) + "_" +
           std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

/// RAII helper: creates shm manually (without GptpIpcPublisher) for edge-case
/// testing; cleans up in destructor.
struct ManualShm
{
    std::string name;
    void* ptr = MAP_FAILED;
    std::size_t size = sizeof(GptpIpcRegion);

    explicit ManualShm(const std::string& n) : name{n}
    {
        const int fd = ::shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd < 0)
            return;
        if (::ftruncate(fd, static_cast<off_t>(size)) != 0)
        {
            ::close(fd);
            return;
        }
        ptr = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ::close(fd);
    }

    ~ManualShm()
    {
        if (ptr != MAP_FAILED)
            ::munmap(ptr, size);
        ::shm_unlink(name.c_str());
    }

    bool Valid() const
    {
        return ptr != MAP_FAILED;
    }
    GptpIpcRegion* Region()
    {
        return static_cast<GptpIpcRegion*>(ptr);
    }
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TS_CLIENT_SRC_GPTP_IPC_TEST_UTILS_H
