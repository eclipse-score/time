/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#include <gtest/gtest.h>
#include "score_time/ipc/shared_state.hpp"

#include <atomic>
#include <thread>

namespace {

using namespace score_time;
using namespace score_time::ipc;

// ─── Construction ────────────────────────────────────────────

TEST(SharedStateTest, DefaultConstruction_MagicIsCorrect)
{
    SharedState s{};
    EXPECT_EQ(s.magic.load(), SharedState::kMagic);
}

TEST(SharedStateTest, DefaultConstruction_VersionIsCorrect)
{
    SharedState s{};
    EXPECT_EQ(s.version.load(), SharedState::kVersion);
}

TEST(SharedStateTest, DefaultConstruction_StructSizeIsCorrect)
{
    SharedState s{};
    EXPECT_EQ(s.struct_size.load(), static_cast<std::uint32_t>(sizeof(SharedState)));
}

TEST(SharedStateTest, DefaultConstruction_VehicleAccIsNoTime)
{
    SharedState s{};
    EXPECT_EQ(s.vehicle_acc.load(), AccuracyQualifier::kNoTime);
}

TEST(SharedStateTest, DefaultConstruction_AbsAccIsInaccuracyNotAvailable)
{
    SharedState s{};
    EXPECT_EQ(s.abs_acc.load(), AbsoluteAccuracyQualifier::kInaccuracyNotAvailable);
}

TEST(SharedStateTest, DefaultConstruction_AbsSecIsNoTimeAvailable)
{
    SharedState s{};
    EXPECT_EQ(s.abs_sec.load(), AbsoluteSecurityQualifier::kNoTimeAvailable);
}

TEST(SharedStateTest, DefaultConstruction_SeqCountersAreZero)
{
    SharedState s{};
    EXPECT_EQ(s.vehicle_seq.load(), 0u);
    EXPECT_EQ(s.abs_seq.load(), 0u);
}

TEST(SharedStateTest, DefaultConstruction_LogHeadsAreZero)
{
    SharedState s{};
    EXPECT_EQ(s.vehicle_log_head.load(), 0u);
    EXPECT_EQ(s.abs_log_head.load(), 0u);
}

// ─── WriteVehicle / ReadVehicle ──────────────────────────────

TEST(SharedStateTest, WriteReadVehicle_RoundTrip_MatchesWrittenValues)
{
    SharedState s{};
    WriteVehicle(s, 1'000'000'000, 2'000'000'000,
                 AccuracyQualifier::kSynchronized, TimePointQualifier::kASIL_B,
                 42, 100);

    std::int64_t bv{}, bm{};
    AccuracyQualifier acc{};
    TimePointQualifier tpq{};
    ASSERT_TRUE(ReadVehicle(s, bv, bm, acc, tpq));
    EXPECT_EQ(bv, 1'000'000'000);
    EXPECT_EQ(bm, 2'000'000'000);
    EXPECT_EQ(acc, AccuracyQualifier::kSynchronized);
    EXPECT_EQ(tpq, TimePointQualifier::kASIL_B);
}

TEST(SharedStateTest, WriteVehicle_ZeroValues_RoundTrip)
{
    SharedState s{};
    WriteVehicle(s, 0, 0, AccuracyQualifier::kNoTime, TimePointQualifier::kQM, 0, 0);

    std::int64_t bv{-1}, bm{-1};
    AccuracyQualifier acc{AccuracyQualifier::kSynchronized};
    TimePointQualifier tpq{TimePointQualifier::kASIL_B};
    ASSERT_TRUE(ReadVehicle(s, bv, bm, acc, tpq));
    EXPECT_EQ(bv, 0);
    EXPECT_EQ(bm, 0);
    EXPECT_EQ(acc, AccuracyQualifier::kNoTime);
    EXPECT_EQ(tpq, TimePointQualifier::kQM);
}

TEST(SharedStateTest, WriteVehicle_AllAccuracyQualifiers_RoundTrip)
{
    const AccuracyQualifier qualifiers[] = {
        AccuracyQualifier::kNoTime,
        AccuracyQualifier::kNotSynchronized,
        AccuracyQualifier::kSynchronized,
        AccuracyQualifier::kUnstable,
        AccuracyQualifier::kTimeJumpDetected,
    };

    for (const auto q : qualifiers)
    {
        SharedState s{};
        WriteVehicle(s, 0, 0, q, TimePointQualifier::kQM, 0, 0);

        std::int64_t bv{}, bm{};
        AccuracyQualifier acc{};
        TimePointQualifier tpq{};
        ASSERT_TRUE(ReadVehicle(s, bv, bm, acc, tpq));
        EXPECT_EQ(acc, q);
    }
}

TEST(SharedStateTest, WriteVehicle_SeqCounter_IsEvenAndIncrementedByTwo)
{
    SharedState s{};
    const auto before = s.vehicle_seq.load();
    WriteVehicle(s, 0, 0, AccuracyQualifier::kNoTime, TimePointQualifier::kQM, 0, 0);
    const auto after = s.vehicle_seq.load();
    EXPECT_EQ(after, before + 2);
    EXPECT_EQ(after % 2, 0u);
}

TEST(SharedStateTest, WriteVehicle_MultipleWrites_LatestValueVisible)
{
    SharedState s{};
    WriteVehicle(s, 1, 2, AccuracyQualifier::kSynchronized, TimePointQualifier::kASIL_B, 3, 4);
    WriteVehicle(s, 5, 6, AccuracyQualifier::kNotSynchronized, TimePointQualifier::kQM, 7, 8);

    std::int64_t bv{}, bm{};
    AccuracyQualifier acc{};
    TimePointQualifier tpq{};
    ASSERT_TRUE(ReadVehicle(s, bv, bm, acc, tpq));
    EXPECT_EQ(bv, 5);
    EXPECT_EQ(acc, AccuracyQualifier::kNotSynchronized);
    EXPECT_EQ(s.vehicle_seq.load(), 4u);
}

TEST(SharedStateTest, WriteVehicle_LastOffsetAndPathDelay_Stored)
{
    SharedState s{};
    WriteVehicle(s, 0, 0, AccuracyQualifier::kNoTime, TimePointQualifier::kQM, 12345, 67890);
    EXPECT_EQ(s.vehicle_last_offset_ns.load(), 12345);
    EXPECT_EQ(s.vehicle_path_delay_ns.load(), 67890);
}

// ─── WriteAbsolute / ReadAbsolute ────────────────────────────

TEST(SharedStateTest, WriteReadAbsolute_RoundTrip_MatchesWrittenValues)
{
    SharedState s{};
    WriteAbsolute(s,
                  1'700'000'000'000'000'000LL,
                  500'000'000'000LL,
                  AbsoluteAccuracyQualifier::kInaccLessThan1s,
                  AbsoluteSecurityQualifier::kTrustworthy,
                  900'000'000LL,
                  100'000LL,
                  50'000LL,
                  400'000'000'000LL,
                  1U);

    std::int64_t utc{}, mono{}, inacc{};
    AbsoluteAccuracyQualifier acc{};
    AbsoluteSecurityQualifier sec{};
    std::uint8_t src{};
    ASSERT_TRUE(ReadAbsolute(s, utc, mono, acc, sec, inacc, src));
    EXPECT_EQ(utc, 1'700'000'000'000'000'000LL);
    EXPECT_EQ(mono, 500'000'000'000LL);
    EXPECT_EQ(acc, AbsoluteAccuracyQualifier::kInaccLessThan1s);
    EXPECT_EQ(sec, AbsoluteSecurityQualifier::kTrustworthy);
    EXPECT_EQ(inacc, 900'000'000LL);
    EXPECT_EQ(src, 1U);
}

TEST(SharedStateTest, WriteAbsolute_SeqCounter_IsEvenAndIncrementedByTwo)
{
    SharedState s{};
    const auto before = s.abs_seq.load();
    WriteAbsolute(s, 0, 0,
                  AbsoluteAccuracyQualifier::kInaccuracyNotAvailable,
                  AbsoluteSecurityQualifier::kNoTimeAvailable,
                  0, 0, 0, 0, 0);
    const auto after = s.abs_seq.load();
    EXPECT_EQ(after, before + 2);
    EXPECT_EQ(after % 2, 0u);
}

// ─── LogVehicle ──────────────────────────────────────────────

TEST(SharedStateTest, LogVehicle_IncrementsHead)
{
    SharedState s{};
    EXPECT_EQ(s.vehicle_log_head.load(), 0u);
    LogVehicle(s, 1000, SyncLogEvent::kVehicleState, 1, 2);
    EXPECT_EQ(s.vehicle_log_head.load(), 1u);
    LogVehicle(s, 2000, SyncLogEvent::kVehicleOffset, 3, 4);
    EXPECT_EQ(s.vehicle_log_head.load(), 2u);
}

TEST(SharedStateTest, LogVehicle_StoresCorrectEntry)
{
    SharedState s{};
    LogVehicle(s, 999, SyncLogEvent::kVehiclePeerDelay, 77, 88);

    const auto &entry = s.vehicle_log[0];
    EXPECT_EQ(entry.monotonic_ns.load(), 999);
    EXPECT_EQ(entry.type.load(), static_cast<std::uint16_t>(SyncLogEvent::kVehiclePeerDelay));
    EXPECT_EQ(entry.v1.load(), 77);
    EXPECT_EQ(entry.v2.load(), 88);
    EXPECT_EQ(entry.seq.load() % 2, 0u);  // even = readable
}

TEST(SharedStateTest, LogVehicle_WrapAround_DoesNotCrash)
{
    SharedState s{};
    for (std::size_t i = 0; i < kVehicleLogCapacity + 10; ++i)
    {
        LogVehicle(s, static_cast<std::int64_t>(i), SyncLogEvent::kVehicleState, 0, 0);
    }
    EXPECT_EQ(s.vehicle_log_head.load(), kVehicleLogCapacity + 10);
}

TEST(SharedStateTest, LogVehicle_WrapAround_OverwritesSlotCorrectly)
{
    SharedState s{};
    // Fill buffer exactly once
    for (std::size_t i = 0; i < kVehicleLogCapacity; ++i)
    {
        LogVehicle(s, static_cast<std::int64_t>(i), SyncLogEvent::kVehicleState, 0, 0);
    }
    // Write one more — overwrites slot 0
    LogVehicle(s, 9999, SyncLogEvent::kVehicleOffset, 42, 43);
    EXPECT_EQ(s.vehicle_log[0].monotonic_ns.load(), 9999);
    EXPECT_EQ(s.vehicle_log[0].v1.load(), 42);
}

// ─── LogAbsolute ─────────────────────────────────────────────

TEST(SharedStateTest, LogAbsolute_IncrementsHead)
{
    SharedState s{};
    LogAbsolute(s, 1000, SyncLogEvent::kAbsState, 0, 0);
    EXPECT_EQ(s.abs_log_head.load(), 1u);
}

TEST(SharedStateTest, LogAbsolute_WrapAround_DoesNotCrash)
{
    SharedState s{};
    for (std::size_t i = 0; i < kAbsLogCapacity + 5; ++i)
    {
        LogAbsolute(s, static_cast<std::int64_t>(i), SyncLogEvent::kAbsUpdate, 0, 0);
    }
    EXPECT_EQ(s.abs_log_head.load(), kAbsLogCapacity + 5);
}

// ─── Concurrent Read/Write ────────────────────────────────────

TEST(SharedStateTest, ConcurrentReadWrite_Vehicle_NoTornReads)
{
    SharedState s{};
    constexpr int kIterations = 5'000;
    std::atomic<bool> stop{false};

    std::thread writer([&] {
        for (int i = 0; i < kIterations; ++i)
        {
            WriteVehicle(s,
                         static_cast<std::int64_t>(i),
                         static_cast<std::int64_t>(i) * 2,
                         AccuracyQualifier::kSynchronized,
                         TimePointQualifier::kASIL_B,
                         i, i);
        }
        stop.store(true, std::memory_order_release);
    });

    int torn_reads = 0;
    while (!stop.load(std::memory_order_acquire))
    {
        std::int64_t bv{}, bm{};
        AccuracyQualifier acc{};
        TimePointQualifier tpq{};
        if (ReadVehicle(s, bv, bm, acc, tpq))
        {
            // Invariant: base_mono_ns == base_vehicle_ns * 2
            if (bm != bv * 2)
                ++torn_reads;
        }
    }
    writer.join();
    EXPECT_EQ(torn_reads, 0) << "Seqlock torn reads detected";
}

}  // namespace
