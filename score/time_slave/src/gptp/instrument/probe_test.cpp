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
#include "score/time_slave/src/gptp/instrument/probe.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

namespace score
{
namespace ts
{
namespace details
{

// ProbeManager is a singleton; reset it between tests.
class ProbeManagerTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        ProbeManager::Instance().SetEnabled(false);
        ProbeManager::Instance().SetRecorder(nullptr);
    }
};

// ── Enable / disable ──────────────────────────────────────────────────────────

TEST_F(ProbeManagerTest, DefaultState_IsDisabled)
{
    EXPECT_FALSE(ProbeManager::Instance().IsEnabled());
}

TEST_F(ProbeManagerTest, SetEnabled_True_IsEnabledReturnsTrue)
{
    ProbeManager::Instance().SetEnabled(true);
    EXPECT_TRUE(ProbeManager::Instance().IsEnabled());
}

TEST_F(ProbeManagerTest, SetEnabled_FalseThenTrue_TogglesCorrectly)
{
    ProbeManager::Instance().SetEnabled(true);
    ProbeManager::Instance().SetEnabled(false);
    EXPECT_FALSE(ProbeManager::Instance().IsEnabled());
}

TEST_F(ProbeManagerTest, Instance_ReturnsSameSingleton)
{
    EXPECT_EQ(&ProbeManager::Instance(), &ProbeManager::Instance());
}

// ── Trace when disabled ───────────────────────────────────────────────────────

TEST_F(ProbeManagerTest, Trace_WhenDisabled_DoesNotCrash)
{
    ProbeData d{};
    d.ts_mono_ns = 1'000'000LL;
    d.value_ns = 500LL;
    d.seq_id = 1U;
    EXPECT_NO_THROW(ProbeManager::Instance().Trace(ProbePoint::kSyncFrameParsed, d));
}

// ── Trace when enabled without recorder ───────────────────────────────────────

TEST_F(ProbeManagerTest, Trace_WhenEnabled_NoRecorder_DoesNotCrash)
{
    ProbeManager::Instance().SetEnabled(true);
    ProbeData d{};
    d.ts_mono_ns = 2'000'000LL;
    d.value_ns = -100LL;
    d.seq_id = 2U;
    EXPECT_NO_THROW(ProbeManager::Instance().Trace(ProbePoint::kFollowUpProcessed, d));
}

// ── Trace with recorder attached ─────────────────────────────────────────────

class ProbeManagerWithRecorderTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        path_ = "/tmp/probe_test_" + std::to_string(::getpid()) + ".csv";
        Recorder::Config cfg;
        cfg.enabled = true;
        cfg.file_path = path_;
        recorder_ = std::make_unique<Recorder>(cfg);

        ProbeManager::Instance().SetEnabled(true);
        ProbeManager::Instance().SetRecorder(recorder_.get());
    }

    void TearDown() override
    {
        ProbeManager::Instance().SetEnabled(false);
        ProbeManager::Instance().SetRecorder(nullptr);
        std::remove(path_.c_str());
    }

    std::string path_;
    std::unique_ptr<Recorder> recorder_;
};

TEST_F(ProbeManagerWithRecorderTest, Trace_WritesToRecorder)
{
    ProbeData d{};
    d.ts_mono_ns = 3'000'000LL;
    d.value_ns = 42LL;
    d.seq_id = 3U;
    ProbeManager::Instance().Trace(ProbePoint::kPdelayCompleted, d);

    // Flush by replacing recorder (which closes file in destructor)
    ProbeManager::Instance().SetRecorder(nullptr);
    recorder_.reset();

    // File should have header + 1 data line
    std::ifstream f(path_);
    int lines = 0;
    std::string line;
    while (std::getline(f, line))
        ++lines;
    EXPECT_EQ(lines, 2);
}

TEST_F(ProbeManagerWithRecorderTest, Trace_AllProbePoints_DoNotCrash)
{
    const ProbePoint points[] = {
        ProbePoint::kRxPacketReceived,
        ProbePoint::kSyncFrameParsed,
        ProbePoint::kFollowUpProcessed,
        ProbePoint::kOffsetComputed,
        ProbePoint::kPdelayReqSent,
        ProbePoint::kPdelayCompleted,
        ProbePoint::kPhcAdjusted,
    };
    for (auto p : points)
    {
        EXPECT_NO_THROW(ProbeManager::Instance().Trace(p, ProbeData{}));
    }
}

// ── ProbeMonoNs ───────────────────────────────────────────────────────────────

TEST(ProbeMonoNsTest, ReturnsPositiveValue)
{
    EXPECT_GT(ProbeMonoNs(), 0LL);
}

TEST(ProbeMonoNsTest, MonotonicallyIncreasing)
{
    const std::int64_t t1 = ProbeMonoNs();
    const std::int64_t t2 = ProbeMonoNs();
    EXPECT_GE(t2, t1);
}

}  // namespace details
}  // namespace ts
}  // namespace score
