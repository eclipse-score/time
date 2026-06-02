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
#include "score/ts_client/src/gptp_ipc_receiver.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

static constexpr int kMaxRetries = 20;

GptpIpcReceiver::~GptpIpcReceiver()
{
    Close();
}

bool GptpIpcReceiver::Init(const std::string& ipc_name)
{
    if (region_ != nullptr)
        return true;

    shm_fd_ = ::shm_open(ipc_name.c_str(), O_RDONLY, 0);
    if (shm_fd_ < 0)
        return false;

    {
        struct ::stat st{};
        if (::fstat(shm_fd_, &st) != 0 || static_cast<std::size_t>(st.st_size) < sizeof(GptpIpcRegion))
        {
            ::close(shm_fd_);
            shm_fd_ = -1;
            return false;
        }
    }

    void* ptr = ::mmap(nullptr, sizeof(GptpIpcRegion), PROT_READ, MAP_SHARED, shm_fd_, 0);
    if (ptr == MAP_FAILED)
    {
        ::close(shm_fd_);
        shm_fd_ = -1;
        return false;
    }

    region_ = static_cast<const GptpIpcRegion*>(ptr);

    if (region_->magic.load(std::memory_order_acquire) != kGptpIpcMagic)
    {
        Close();
        return false;
    }

    return true;
}

std::optional<score::ts::GptpIpcData> GptpIpcReceiver::Receive()
{
    if (region_ == nullptr)
        return std::nullopt;

    for (int attempt = 0; attempt < kMaxRetries; ++attempt)
    {
        const std::uint32_t seq1 = region_->seq.load(std::memory_order_acquire);

        if ((seq1 & 1U) != 0U)
            continue;  // write in progress, retry

        score::ts::GptpIpcData data{};
        std::memcpy(&data, &region_->data, sizeof(score::ts::GptpIpcData));

        // acq_rel fence: prevents data reads from floating past the consistency checks below
        // (release half prevents memcpy reordering after the fence on ARM64), and prevents
        // the seq/seq_confirm loads below from floating before the data reads (acquire half).
        std::atomic_thread_fence(std::memory_order_acq_rel);

        const std::uint32_t seq2 = region_->seq_confirm.load(std::memory_order_acquire);
        // Re-read seq to detect a write that started AFTER our initial seq1 snapshot.
        // Without this, a writer that sets seq=odd after seq1 was loaded would go undetected:
        // seq_confirm would still hold the old even value, causing the reader to return
        // partially-written data (seqlock race on all multi-core platforms).
        const std::uint32_t seq3 = region_->seq.load(std::memory_order_acquire);

        if (seq1 == seq2 && seq1 == seq3)
            return data;
    }

    return std::nullopt;
}

void GptpIpcReceiver::Close()
{
    if (region_ != nullptr)
    {
        ::munmap(const_cast<GptpIpcRegion*>(region_), sizeof(GptpIpcRegion));
        region_ = nullptr;
    }
    if (shm_fd_ >= 0)
    {
        ::close(shm_fd_);
        shm_fd_ = -1;
    }
}

}  // namespace details
}  // namespace ts
}  // namespace score
