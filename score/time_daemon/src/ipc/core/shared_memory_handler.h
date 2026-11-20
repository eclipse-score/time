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

#include <score/bit.hpp>

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
#include <type_traits>

namespace score
{
namespace td
{

///
/// \brief The class implements shared memory handling
///
/// \details Only one thread at a time, across all processes, may call Send(). A single seqlock guards the
///          payload, which is stored as relaxed atomic bytes so that concurrent reads and writes are
///          well-defined (data-race-free) in C++. Readers always see a consistent, monotonically advancing snapshot.
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
    /// \brief Read a snapshot from shared memory, retrying if a write overlapped the read.
    ///
    std::optional<DataType> Receive() const;

    ///
    /// \brief Write data to shared memory without waiting for readers.
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
    static constexpr bool NoWriteInProgress(std::uint64_t seq)
    {
        return (seq & 1U) == 0U;
    }

    /// \brief Object representation of DataType, used to move the payload through the atomic bytes
    using Bytes = std::array<std::uint8_t, sizeof(DataType)>;

    ///
    /// \brief Seqlock counter and payload, stored as atomic bytes to avoid a C++ data race with the reader
    /// \tparam DataType shall be trivially copyable data
    ///
    struct SharedData
    {
        static_assert(std::is_trivially_copyable_v<DataType>,
                      "DataType must be trivially copyable to be stored in shared memory!");
        static_assert(std::is_default_constructible_v<DataType>,
                      "DataType must be default constructible (required by DataType{} usage)!");
        static_assert(std::is_standard_layout_v<DataType>,
                      "DataType should be standard-layout for robust shared memory/IPC usage!");
        static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                      "Atomics shared between processes must be lock-free!");

        /// \brief seq_ seqlock counter: odd while a write is in progress, even otherwise.
        std::atomic<std::uint64_t> seq_{0U};

        /// \brief Payload bytes (DataType object representation), zero-initialised, accessed only through relaxed
        /// atomic loads/stores.
        std::array<std::atomic<std::uint8_t>, sizeof(DataType)> bytes_{};
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
    if (shared_memory_data_ != nullptr)
    {
        for (std::uint8_t retry_cnt = 0U; retry_cnt < max_number_of_read_retries_; ++retry_cnt)
        {
            const auto seq_before_read = shared_memory_data_->seq_.load(std::memory_order_acquire);

            if (NoWriteInProgress(seq_before_read))
            {
                Bytes bytes{};
                auto source = shared_memory_data_->bytes_.cbegin();
                for (auto& byte : bytes)
                {
                    byte = source->load(std::memory_order_relaxed);
                    ++source;
                }

                // Order the payload loads before the re-check of the counter
                std::atomic_thread_fence(std::memory_order_acquire);

                // Unchanged counter: no write started or completed during the copy
                if (shared_memory_data_->seq_.load(std::memory_order_relaxed) == seq_before_read)
                {
                    return score::cpp::bit_cast<DataType>(bytes);
                }
            }
        }

        score::mw::log::LogError(kIpcHandlerContext)
            << "Read failed for number of retries: " << max_number_of_read_retries_;
    }

    return std::nullopt;
}

template <typename DataType>
void SharedMemoryHandler<DataType>::Send(const DataType& data)
{
    if (shared_memory_data_ != nullptr)
    {
        const auto bytes = score::cpp::bit_cast<Bytes>(data);

        // Single writer: counter becomes odd to signal write in progress
        const auto seq = shared_memory_data_->seq_.load(std::memory_order_relaxed);
        shared_memory_data_->seq_.store(seq + 1U, std::memory_order_relaxed);

        // Order the odd counter before the payload stores
        std::atomic_thread_fence(std::memory_order_release);

        auto destination = shared_memory_data_->bytes_.begin();
        for (const auto byte : bytes)
        {
            destination->store(byte, std::memory_order_relaxed);
            ++destination;
        }

        // Even again: publishes the payload to readers
        shared_memory_data_->seq_.store(seq + 2U, std::memory_order_release);
    }
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_MSG_BROKER_SHARED_DATA_H
