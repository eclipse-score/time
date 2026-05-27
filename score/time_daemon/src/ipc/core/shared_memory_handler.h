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

#include "score/time_daemon/src/common/logging_contexts.h"
#include "score/memory/shared/managed_memory_resource.h"
#include "score/memory/shared/shared_memory_factory.h"
#include "score/memory/shared/shared_memory_resource.h"

#include <atomic>
#include <cstdint>
#include <optional>

namespace score
{
namespace td
{

///
/// \brief The class implements shared memory handling
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
    ///
    /// \brief Common type to store mutex and specified data
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

        std::atomic<std::uint16_t> entry_cnt_{0U};

        /// \brief data_ specific data placed in share dmemory region. Note: It has to be trivially copyable with
        // trivial simple types that are not allocated on cheap!
        DataType data_{};

        /// \brief entry_cnt_ atomic entry counter to notify reader with exit_cnt_ that write occurred during reading
        std::atomic<std::uint16_t> exit_cnt_{0U};
    };

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
            sizeof(SharedData),
            {{permissions}, {}});

        if (shared_memory_resource_ == nullptr)
        {
            score::mw::log::LogFatal(kIpcHandlerContext)
                << "shared memory segment could not be created for path " << shared_memory_path_;
        }
    }

    if ((shared_memory_data_ == nullptr) && (shared_memory_resource_ != nullptr))
    {
        // Shared memory was opened (not created) and now we need to map our struct there
        // cast from void to T* is done by design as long as shared memory is agnostic to the type
        // but clients only know, what is stored there. Also see the score::memory::shared::ManagedMemoryResource
        // design for details.
        shared_memory_data_ = static_cast<SharedData*>(shared_memory_resource_->getUsableBaseAddress());
        score::mw::log::LogInfo(kIpcHandlerContext)
            << "Shared memory object was found. Mapped data in it: " << shared_memory_data_;
    }

    return ((shared_memory_data_ != nullptr) && (shared_memory_resource_ != nullptr));
}

template <typename DataType>
std::optional<DataType> SharedMemoryHandler<DataType>::Receive() const
{
    if (shared_memory_data_ != nullptr)
    {
        DataType read_data{};

        for (std::uint8_t retry_cnt = 0U; retry_cnt < max_number_of_read_retries_; ++retry_cnt)
        {
            // Snapshot entry counter
            auto entry_cnt_before_read = shared_memory_data_->entry_cnt_.load(std::memory_order_acquire);

            // Copy the payload
            read_data = shared_memory_data_->data_;

            // Snapshot exit counter
            auto exit_cnt_after_read = shared_memory_data_->exit_cnt_.load(std::memory_order_acquire);

            // Check if no data update happened during read
            if (entry_cnt_before_read == exit_cnt_after_read)
            {
                return read_data;
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
        // Signal start of write (sequentially consistent by default)
        std::ignore = shared_memory_data_->entry_cnt_.fetch_add(1U);

        // Copy the non-atomic payload
        shared_memory_data_->data_ = data;

        //  Publish completion (guarantees visibility of payload)
        shared_memory_data_->exit_cnt_.store(shared_memory_data_->entry_cnt_.load());
    }
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_MSG_BROKER_SHARED_DATA_H
