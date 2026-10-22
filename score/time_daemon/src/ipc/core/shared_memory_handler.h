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
#ifndef SCORE_TIME_DAEMON_SRC_MSG_BROKER_SHARED_DATA_H
#define SCORE_TIME_DAEMON_SRC_MSG_BROKER_SHARED_DATA_H

#include "score/memory/shared/managed_memory_resource.h"
#include "score/memory/shared/shared_memory_factory.h"
#include "score/memory/shared/shared_memory_resource.h"
#include "score/time_daemon/src/common/logging_contexts.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>

namespace score
{
namespace td
{

///
/// \brief The class implements shared memory handling
///
/// \details One writer sends the latest DataType to any number of readers, also in other processes. It uses
///          read-copy-update (RCU) over a fixed number of slots: https://lwn.net/Articles/262464/
///          - Send() writes into a slot that is not published and has no readers, then publishes it.
///          - Receive() adds 1 to the slot's reader count, checks that the slot is still published, copies the data
///            and removes 1 again.
///          Only one thread at a time, across all processes, may call Send().
///
template <typename DataType>
class SharedMemoryHandler
{
  public:
    explicit SharedMemoryHandler(const std::string& shared_memory_path)
        : shared_memory_path_{shared_memory_path},
          shared_memory_resource_{},
          shared_memory_data_{nullptr},
          max_number_of_read_retries_{10U}
    {
    }

    ///
    /// \brief Initialize shared memory
    /// \return true -> init succeeded
    ///
    bool Init();

    ///
    /// \brief Safely read data from shared memory.
    ///
    std::optional<DataType> Receive() const;

    ///
    /// \brief Safely write data to shared memory
    ///
    void Send(const DataType& data);

    ~SharedMemoryHandler()
    {
        shared_memory_resource_.reset();
    }

    SharedMemoryHandler(const SharedMemoryHandler&) = delete;
    SharedMemoryHandler(SharedMemoryHandler&&) = delete;
    SharedMemoryHandler& operator=(const SharedMemoryHandler&) = delete;
    SharedMemoryHandler& operator=(SharedMemoryHandler&&) = delete;

  private:
    /// \brief Assumed CPU cache-line size in bytes.
    /// Used to keep data that different cores write on separate cache lines to reduce false sharing.
    static constexpr std::size_t kCacheLineSize{64U};

    /// \brief Number of copies of the data in shared memory. Send() needs a slot that is not published and has no
    /// readers, so more slots let more readers be stuck in Receive() before Send() has to drop data.
    static constexpr std::uint32_t kNumberOfSlots{10U};

    /// \brief One copy of the data. Starts on a new cache line.
    struct alignas(kCacheLineSize) Slot
    {
        DataType data_{};
    };

    ///
    /// \brief Everything in shared memory: the slots and the data that manages them
    /// \tparam DataType shall be trivially copyable data
    ///
    struct alignas(kCacheLineSize) SharedData
    {
        static_assert(std::is_trivially_copyable_v<DataType>,
                      "DataType must be trivially copyable to be stored in shared memory!");
        static_assert(std::is_default_constructible_v<DataType>,
                      "DataType must be default constructible (required by DataType{} usage)!");
        static_assert(std::is_standard_layout_v<DataType>,
                      "DataType should be standard-layout for robust shared memory/IPC usage!");
        static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                      "Atomics shared between processes must be lock-free!");

        /// \brief published_slot_ index of the slot that Receive() reads. Only Send() writes it.
        std::atomic<std::uint64_t> published_slot_{0U};

        /// \brief readers_per_slot_ readers per slot, on its own cache line. Readers write nothing else.
        alignas(kCacheLineSize) std::array<std::atomic<std::uint64_t>, kNumberOfSlots> readers_per_slot_{};

        /// \brief slots_ copies of the data.
        std::array<Slot, kNumberOfSlots> slots_{};
    };

    /// \brief Bytes to reserve. Aligning SharedData can add up to alignof(SharedData) bytes of padding.
    static constexpr std::size_t kSharedMemorySize{sizeof(SharedData) + alignof(SharedData)};

    const std::string shared_memory_path_;
    std::shared_ptr<score::memory::shared::ManagedMemoryResource> shared_memory_resource_;
    SharedMemoryHandler::SharedData* shared_memory_data_;
    const std::size_t max_number_of_read_retries_;
};

template <typename DataType>
bool SharedMemoryHandler<DataType>::Init()
{
    if (shared_memory_resource_ == nullptr)
    {
        score::memory::shared::SharedMemoryFactory::WorldWritable permissions{};
        shared_memory_resource_ = score::memory::shared::SharedMemoryFactory::CreateOrOpen(
            shared_memory_path_,
            [this](std::shared_ptr<score::memory::shared::ISharedMemoryResource> memory_resource) {
                shared_memory_data_ = memory_resource->construct<SharedData>();
            },
            kSharedMemorySize,
            {{permissions}, {}});

        if (shared_memory_resource_ == nullptr)
        {
            score::mw::log::LogFatal(kIpcHandlerContext)
                << "shared memory segment could not be created for path " << shared_memory_path_;
        }
    }

    if ((shared_memory_data_ == nullptr) && (shared_memory_resource_ != nullptr))
    {
        // Shared memory was opened (not created) and now we need to map our struct there.
        // The creator placed SharedData first, aligned to alignof(SharedData). Use the same alignment to find it.
        void* address = shared_memory_resource_->getUsableBaseAddress();
        std::size_t space = shared_memory_resource_->GetUserAllocatedBytes();
        shared_memory_data_ =
            static_cast<SharedData*>(std::align(alignof(SharedData), sizeof(SharedData), address, space));

        if (shared_memory_data_ == nullptr)
        {
            score::mw::log::LogError(kIpcHandlerContext)
                << "Shared memory object " << shared_memory_path_ << " is too small for the expected data layout";
        }
        else
        {
            score::mw::log::LogInfo(kIpcHandlerContext)
                << "Shared memory object was found. Mapped data in it: " << shared_memory_data_;
        }
    }

    return ((shared_memory_data_ != nullptr) && (shared_memory_resource_ != nullptr));
}

template <typename DataType>
std::optional<DataType> SharedMemoryHandler<DataType>::Receive() const
{
    std::optional<DataType> read_data{};

    if (shared_memory_data_ != nullptr)
    {
        for (std::uint8_t retry_cnt = 0U; retry_cnt < max_number_of_read_retries_; ++retry_cnt)
        {
            // The modulo keeps the index inside the array.
            const auto slot = shared_memory_data_->published_slot_.load(std::memory_order_relaxed) % kNumberOfSlots;
            auto& slot_readers = shared_memory_data_->readers_per_slot_[slot];

            // Add 1 to the reader count, then check that the slot is still published. If it is, Send() does not
            // write into it until we remove 1 again.
            std::ignore = slot_readers.fetch_add(1U, std::memory_order_seq_cst);
            if (shared_memory_data_->published_slot_.load(std::memory_order_seq_cst) == slot)
            {
                read_data.emplace(shared_memory_data_->slots_[slot].data_);

                // Remove 1 from the reader count. Release makes sure the copy is done before Send() writes here again.
                std::ignore = slot_readers.fetch_sub(1U, std::memory_order_release);
                return read_data;
            }

            // Send() published another slot in the meantime. Remove 1 from the reader count and try the new slot.
            std::ignore = slot_readers.fetch_sub(1U, std::memory_order_relaxed);
        }

        score::mw::log::LogError(kIpcHandlerContext)
            << "Read failed for number of retries: " << max_number_of_read_retries_;
    }

    return read_data;
}

template <typename DataType>
void SharedMemoryHandler<DataType>::Send(const DataType& data)
{
    if (shared_memory_data_ != nullptr)
    {
        // Only Send() writes published_slot_, so this reads the value we wrote last time
        const auto published_slot = shared_memory_data_->published_slot_.load(std::memory_order_relaxed);

        // Start with the slot after the published one, which is usually the oldest. Never use the published slot.
        for (std::uint32_t offset = 1U; offset < kNumberOfSlots; ++offset)
        {
            const auto candidate = (published_slot + offset) % kNumberOfSlots;
            if (shared_memory_data_->readers_per_slot_[candidate].load(std::memory_order_seq_cst) == 0U)
            {
                shared_memory_data_->slots_[candidate].data_ = data;
                shared_memory_data_->published_slot_.store(candidate, std::memory_order_seq_cst);
                return;
            }
        }

        score::mw::log::LogError(kIpcHandlerContext) << "All slots are being read, data was not published";
    }
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_MSG_BROKER_SHARED_DATA_H
