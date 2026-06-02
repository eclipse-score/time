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
#include "score/ts_client/src/gptp_ipc_publisher.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <type_traits>

static_assert(std::is_trivially_copyable<score::ts::GptpIpcData>::value,
              "GptpIpcData must be trivially copyable for seqlock memcpy to be valid");

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
    if (region_ != nullptr)
        return true;

    ipc_name_ = ipc_name;

    (void)::shm_unlink(ipc_name_.c_str());

    shm_fd_ = ::shm_open(ipc_name_.c_str(), O_CREAT | O_RDWR, 0600);
    if (shm_fd_ < 0)
        return false;

    if (::ftruncate(shm_fd_, static_cast<off_t>(sizeof(GptpIpcRegion))) != 0)
    {
        ::close(shm_fd_);  // LCOV_EXCL_LINE
        shm_fd_ = -1;      // LCOV_EXCL_LINE
        return false;      // LCOV_EXCL_LINE
    }

    void* ptr = ::mmap(nullptr, sizeof(GptpIpcRegion), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
    if (ptr == MAP_FAILED)
    {
        ::close(shm_fd_);  // LCOV_EXCL_LINE
        shm_fd_ = -1;      // LCOV_EXCL_LINE
        return false;      // LCOV_EXCL_LINE
    }

    region_ = new (ptr) GptpIpcRegion{};
    return true;
}

void GptpIpcPublisher::Publish(const score::ts::GptpIpcData& data)
{
    if (region_ == nullptr)
        return;

    const std::uint32_t next = region_->seq.load(std::memory_order_relaxed) + 1U;
    region_->seq.store(next, std::memory_order_relaxed);
    // Release fence: prevents the data writes below from being reordered before
    // the seq=odd store above on weakly-ordered CPUs (ARM64/QNX).  The acquire
    // half of acq_rel is unnecessary for a seqlock writer; release suffices here.
    std::atomic_thread_fence(std::memory_order_release);

    std::memcpy(&region_->data, &data, sizeof(score::ts::GptpIpcData));

    region_->seq_confirm.store(next + 1U, std::memory_order_release);
    region_->seq.store(next + 1U, std::memory_order_release);
}

void GptpIpcPublisher::Destroy()
{
    if (region_ != nullptr)
    {
        region_->~GptpIpcRegion();
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
