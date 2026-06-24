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
#include "score/memory/shared/shared_memory_factory.h"

#include <unistd.h>
#include <atomic>
#include <memory>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

inline std::string UniqueShmName()
{
    static std::atomic<int> counter{0};
    return "/gptp_ipc_ut_" + std::to_string(::getpid()) + "_" +
           std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

/// RAII helper: creates SHM via SharedMemoryFactory (same layout as GptpIpcPublisher)
/// so that GptpIpcReceiver can open it.  Gives direct access to the region for
/// edge-case tests that need to corrupt seq/magic.
struct ManualShm
{
    std::shared_ptr<score::memory::shared::ISharedMemoryResource> resource_;
    GptpIpcRegion* region_{nullptr};
    std::string name_;

    explicit ManualShm(const std::string& n) : name_{n}
    {
        score::memory::shared::SharedMemoryFactory::Remove(n);
        score::memory::shared::SharedMemoryFactory::RemoveStaleArtefacts(n);
        resource_ = score::memory::shared::SharedMemoryFactory::Create(
            n,
            [this](std::shared_ptr<score::memory::shared::ISharedMemoryResource> res) {
                region_ = res->construct<GptpIpcRegion>();
            },
            sizeof(GptpIpcRegion) + alignof(GptpIpcRegion) - 1U,
            score::memory::shared::permission::WorldWritable{});
    }

    ~ManualShm()
    {
        resource_.reset();
        score::memory::shared::SharedMemoryFactory::Remove(name_);
    }

    bool Valid() const { return resource_ != nullptr && region_ != nullptr; }
    GptpIpcRegion* Region() { return region_; }
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TS_CLIENT_SRC_GPTP_IPC_TEST_UTILS_H
