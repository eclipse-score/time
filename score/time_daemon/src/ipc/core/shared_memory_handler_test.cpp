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
#include "score/time_daemon/src/ipc/core/shared_memory_handler.h"
#include "score/time_daemon/src/ipc/core/test_types.h"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace score
{
namespace td
{
namespace
{

///
/// \brief The writer sets every payload element to the same sequence number,
/// so a snapshot with differing elements mixes two writes (torn read).
///
struct SequencePayload
{
    std::array<std::uint64_t, 64U> data;
};

SequencePayload MakeSequencePayload(const std::uint64_t sequence)
{
    SequencePayload payload{};
    payload.data.fill(sequence);
    return payload;
}

bool IsConsistent(const SequencePayload& payload)
{
    for (const auto word : payload.data)
    {
        if (word != payload.data.front())
        {
            return false;
        }
    }
    return true;
}

}  // namespace

class SharedMemoryHandlerTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        score::memory::shared::SharedMemoryFactory::RemoveStaleArtefacts(shared_memory_path_);
    }

    void TearDown() override
    {
        score::memory::shared::SharedMemoryFactory::RemoveStaleArtefacts(shared_memory_path_);
    }

    const std::string shared_memory_path_{"/sh_some_data"};
};

TEST_F(SharedMemoryHandlerTest, TestReadAndWrite)
{

    // For given shared memory path create handler
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);

    // Then initialize and map shm resource
    EXPECT_TRUE(handler.Init());

    // Write some data
    test::FakeTimeInfoIpc input_data = {123, 456};
    EXPECT_NO_THROW(handler.Send(input_data));

    // Then expect that obtained data is equal to simulated.
    auto data = handler.Receive();
    EXPECT_TRUE(data.has_value());
    EXPECT_EQ(data.value(), input_data);
}

TEST_F(SharedMemoryHandlerTest, TestWriteWithoutInit)
{
    // For given shared memory path create handler
    auto handler = SharedMemoryHandler<test::FakeTimeInfoIpc>(shared_memory_path_);

    // Write some data
    test::FakeTimeInfoIpc input_data = {123, 456};
    EXPECT_NO_THROW(handler.Send(input_data));

    // Then expect that obtained data is not equal to simulated
    auto data = handler.Receive();
    EXPECT_FALSE(data.has_value());
}

TEST_F(SharedMemoryHandlerTest, TestConcurrentWriteAndReadsReturnConsistentData)
{
    constexpr std::uint64_t kNumberOfWrites{100'000U};
    constexpr std::size_t kNumberOfReaders{10U};

    auto writer = SharedMemoryHandler<SequencePayload>(shared_memory_path_);
    ASSERT_TRUE(writer.Init());

    std::array<std::unique_ptr<SharedMemoryHandler<SequencePayload>>, kNumberOfReaders> readers{};
    for (auto& reader : readers)
    {
        reader = std::make_unique<SharedMemoryHandler<SequencePayload>>(shared_memory_path_);
        ASSERT_TRUE(reader->Init());
    }

    std::atomic<std::size_t> readers_ready{0U};
    std::atomic<bool> writer_done{false};
    std::array<std::uint64_t, kNumberOfReaders> successful_reads{};
    std::array<std::uint64_t, kNumberOfReaders> torn_reads{};
    std::array<std::uint64_t, kNumberOfReaders> out_of_order_reads{};

    std::vector<std::thread> reader_threads{};
    for (std::size_t index = 0U; index < kNumberOfReaders; ++index)
    {
        reader_threads.emplace_back([&, index]() {
            std::uint64_t last_sequence{0U};
            std::ignore = readers_ready.fetch_add(1U);
            while (!writer_done.load())
            {
                const auto snapshot = readers[index]->Receive();
                if (!snapshot.has_value())
                {
                    continue;
                }

                ++successful_reads[index];
                if (!IsConsistent(snapshot.value()))
                {
                    ++torn_reads[index];
                    continue;
                }

                const auto sequence = snapshot.value().data.front();
                if (sequence < last_sequence)
                {
                    ++out_of_order_reads[index];
                }
                last_sequence = sequence;
            }
        });
    }

    // Wait for all readers to be ready before starting the writer
    while (readers_ready.load() < kNumberOfReaders)
    {
        std::this_thread::yield();
    }

    for (std::uint64_t sequence = 1U; sequence <= kNumberOfWrites; ++sequence)
    {
        writer.Send(MakeSequencePayload(sequence));
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    writer_done.store(true);

    for (auto& thread : reader_threads)
    {
        thread.join();
    }

    for (std::size_t index = 0U; index < kNumberOfReaders; ++index)
    {
        std::cout << "reader " << index << ": successful=" << successful_reads[index]
                  << " torn=" << torn_reads[index] << " out_of_order=" << out_of_order_reads[index] << std::endl;

        EXPECT_GT(successful_reads[index], 0U) << "reader " << index;
        EXPECT_EQ(torn_reads[index], 0U) << "reader " << index << " of " << successful_reads[index] << " reads";
        EXPECT_EQ(out_of_order_reads[index], 0U) << "reader " << index;

        // Every reader must see the last written data
        const auto final_snapshot = readers[index]->Receive();
        ASSERT_TRUE(final_snapshot.has_value()) << "reader " << index;
        std::cout << "reader " << index << ": final_sequence=" << final_snapshot.value().data.front() << std::endl;
        EXPECT_EQ(final_snapshot.value().data, MakeSequencePayload(kNumberOfWrites).data) << "reader " << index;
    }
}

}  // namespace td
}  // namespace score
