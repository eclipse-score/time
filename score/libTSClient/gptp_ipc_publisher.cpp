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
#include "score/libTSClient/gptp_ipc_publisher.h"

#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace score
{
namespace ts
{
namespace details
{

GptpIpcPublisher::~GptpIpcPublisher()
{
    Destroy();
}

bool GptpIpcPublisher::Init(const std::string& ipc_name)
{
    ipc_name_ = ipc_name;

    shm_fd_ = ::shm_open(ipc_name_.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd_ < 0)
        return false;

    if (::ftruncate(shm_fd_, static_cast<off_t>(sizeof(GptpIpcRegion))) != 0)
    {
        ::close(shm_fd_);          // LCOV_EXCL_LINE
        shm_fd_ = -1;              // LCOV_EXCL_LINE
        return false;              // LCOV_EXCL_LINE
    }

    void* ptr = ::mmap(nullptr, sizeof(GptpIpcRegion),
                       PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
    if (ptr == MAP_FAILED)
    {
        ::close(shm_fd_);          // LCOV_EXCL_LINE
        shm_fd_ = -1;              // LCOV_EXCL_LINE
        return false;              // LCOV_EXCL_LINE
    }

    region_ = new (ptr) GptpIpcRegion{};
    return true;
}

void GptpIpcPublisher::Publish(const score::td::PtpTimeInfo& info)
{
    if (region_ == nullptr)
        return;

    const std::uint32_t next = region_->seq.load(std::memory_order_relaxed) + 1U;
    region_->seq.store(next, std::memory_order_release);

    std::atomic_thread_fence(std::memory_order_release);
    std::memcpy(&region_->data, &info, sizeof(score::td::PtpTimeInfo));
    std::atomic_thread_fence(std::memory_order_release);

    region_->seq_confirm.store(next + 1U, std::memory_order_release);
    region_->seq.store(next + 1U, std::memory_order_release);
}

void GptpIpcPublisher::Destroy()
{
    if (region_ != nullptr)
    {
        ::munmap(region_, sizeof(GptpIpcRegion));
        region_ = nullptr;
    }
    if (shm_fd_ >= 0)
    {
        ::close(shm_fd_);
        shm_fd_ = -1;
    }
    if (!ipc_name_.empty())
    {
        ::shm_unlink(ipc_name_.c_str());
        ipc_name_.clear();
    }
}

}  // namespace details
}  // namespace ts
}  // namespace score
