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
    Close();
}

bool GptpIpcPublisher::Open(const std::string& ipc_name)
{
    if (shm_resource_ != nullptr)
        return true;

    ipc_name_ = ipc_name;

    score::memory::shared::SharedMemoryFactory::Remove(ipc_name_);
    score::memory::shared::SharedMemoryFactory::RemoveStaleArtefacts(ipc_name_);

    shm_resource_ = score::memory::shared::SharedMemoryFactory::Create(
        ipc_name_,
        [this](std::shared_ptr<score::memory::shared::ISharedMemoryResource> res) {
            region_ = res->construct<GptpIpcRegion>();
        },
        sizeof(GptpIpcRegion) + alignof(GptpIpcRegion) - 1U,
        score::memory::shared::permission::WorldWritable{});

    return (shm_resource_ != nullptr) && (region_ != nullptr);
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

void GptpIpcPublisher::Close()
{
    if (!ipc_name_.empty())
    {
        score::memory::shared::SharedMemoryFactory::Remove(ipc_name_);
        ipc_name_.clear();
    }
    shm_resource_.reset();
    region_ = nullptr;
}

}  // namespace details
}  // namespace ts
}  // namespace score
