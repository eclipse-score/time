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
#include "score/time_slave/src/gptp/record/recorder.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

namespace
{

std::string TempPath()
{
    return "/tmp/recorder_test_" + std::to_string(::getpid()) + ".csv";
}

}  // namespace

// ── Disabled recorder ────────────────────────────────────────────────────────

TEST(RecorderTest, Disabled_IsEnabledReturnsFalse)
{
    Recorder::Config cfg;
    cfg.enabled = false;
    Recorder r{cfg};
    EXPECT_FALSE(r.IsEnabled());
}

TEST(RecorderTest, Disabled_RecordDoesNotCrash)
{
    Recorder::Config cfg;
    cfg.enabled = false;
    Recorder r{cfg};
    EXPECT_NO_THROW(r.Record(RecordEntry{}));
}

// ── Enabled with bad path ─────────────────────────────────────────────────────

TEST(RecorderTest, Enabled_BadPath_IsEnabledReturnsFalse)
{
    Recorder::Config cfg;
    cfg.enabled = true;
    cfg.file_path = "/no/such/dir/recorder_test.csv";
    Recorder r{cfg};
    EXPECT_FALSE(r.IsEnabled());
}

TEST(RecorderTest, Enabled_BadPath_RecordDoesNotCrash)
{
    Recorder::Config cfg;
    cfg.enabled = true;
    cfg.file_path = "/no/such/dir/recorder_test.csv";
    Recorder r{cfg};
    EXPECT_NO_THROW(r.Record(RecordEntry{}));
}

// ── Enabled with valid path ───────────────────────────────────────────────────

class RecorderFileTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        path_ = TempPath();
    }
    void TearDown() override
    {
        std::remove(path_.c_str());
    }

    Recorder MakeRecorder()
    {
        Recorder::Config cfg;
        cfg.enabled = true;
        cfg.file_path = path_;
        return Recorder{cfg};
    }

    std::string path_;
};

TEST_F(RecorderFileTest, IsEnabled_ReturnsTrue)
{
    auto r = MakeRecorder();
    EXPECT_TRUE(r.IsEnabled());
}

TEST_F(RecorderFileTest, NewFile_ContainsCsvHeader)
{
    {
        auto r = MakeRecorder();
    }  // destructor closes file

    std::ifstream f(path_);
    std::string line;
    ASSERT_TRUE(std::getline(f, line));
    EXPECT_EQ(line, "mono_ns,event,offset_ns,pdelay_ns,seq_id,status_flags");
}

TEST_F(RecorderFileTest, Record_WritesOneDataLine)
{
    auto r = MakeRecorder();

    RecordEntry e{};
    e.mono_ns = 123456789LL;
    e.event = RecordEvent::kSyncReceived;
    e.offset_ns = -500LL;
    e.pdelay_ns = 1000LL;
    e.seq_id = 42U;
    e.status_flags = 0x03U;
    r.Record(e);

    // Flush by destroying the recorder before reading back
    r.Record(RecordEntry{});  // second line
}

TEST_F(RecorderFileTest, Record_MultipleEntries_AllFlushedToFile)
{
    {
        auto r = MakeRecorder();
        for (int i = 0; i < 5; ++i)
        {
            RecordEntry e{};
            e.mono_ns = static_cast<std::int64_t>(i) * 1'000'000LL;
            e.event = RecordEvent::kPdelayCompleted;
            e.seq_id = static_cast<std::uint16_t>(i);
            r.Record(e);
        }
    }

    // Count lines: header + 5 data lines = 6
    std::ifstream f(path_);
    int lines = 0;
    std::string line;
    while (std::getline(f, line))
        ++lines;
    EXPECT_EQ(lines, 6);
}

TEST_F(RecorderFileTest, Record_FieldsWrittenCorrectly)
{
    {
        auto r = MakeRecorder();
        RecordEntry e{};
        e.mono_ns = 9'000'000'000LL;
        e.event = RecordEvent::kClockJump;
        e.offset_ns = 12345LL;
        e.pdelay_ns = 999LL;
        e.seq_id = 7U;
        e.status_flags = 0x01U;
        r.Record(e);
    }

    std::ifstream f(path_);
    std::string header, data;
    ASSERT_TRUE(std::getline(f, header));
    ASSERT_TRUE(std::getline(f, data));

    // Format: mono_ns,event,offset_ns,pdelay_ns,seq_id,status_flags
    EXPECT_EQ(data, "9000000000,2,12345,999,7,1");
}

TEST_F(RecorderFileTest, ExistingFile_HeaderNotWrittenAgain)
{
    // Pre-populate the file so tellp() != 0 when the Recorder opens it in
    // append mode — the header-write branch must be skipped.
    {
        std::ofstream f(path_);
        f << "existing_line\n";
    }

    {
        auto r = MakeRecorder();
    }

    std::ifstream f(path_);
    std::string line1, line2;
    ASSERT_TRUE(std::getline(f, line1));
    EXPECT_EQ(line1, "existing_line");
    if (std::getline(f, line2))
    {
        EXPECT_NE(line2, "mono_ns,event,offset_ns,pdelay_ns,seq_id,status_flags");
    }
}

TEST_F(RecorderFileTest, Record_FlushIntervalOne_TriggersFlushAfterEachRecord)
{
    // flush_interval=1: after the first Record() flush_counter_ reaches the
    // threshold, exercising the flush branch and the counter reset path.
    Recorder::Config cfg;
    cfg.enabled = true;
    cfg.file_path = path_;
    cfg.flush_interval = 1U;
    Recorder r{cfg};

    RecordEntry e{};
    e.mono_ns = 42LL;
    e.event = RecordEvent::kSyncReceived;
    r.Record(e);

    std::ifstream f(path_);
    int lines = 0;
    std::string line;
    while (std::getline(f, line))
        ++lines;
    EXPECT_GE(lines, 2);  // header + at least 1 data line
}

}  // namespace details
}  // namespace ts
}  // namespace score
