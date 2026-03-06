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
#include "config_loader.hpp"

#include <cstdio>
#include <fstream>
#include <string>

namespace {

static constexpr const char *kTmpConf = "/tmp/test_config_loader.conf";

static void WriteTmpConf(const std::string &content)
{
    std::ofstream f(kTmpConf);
    f << content;
}

class ConfigLoaderTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        std::remove(kTmpConf);
    }
};

// ─── File Access ─────────────────────────────────────────────

TEST_F(ConfigLoaderTest, NonExistentFile_ReturnsFalse)
{
    tsyncd::EngineOptions opt;
    EXPECT_FALSE(tsyncd::LoadEngineOptionsFromFile("/tmp/no_such_file_xyz12345.conf", opt));
}

TEST_F(ConfigLoaderTest, EmptyFile_ReturnsTrueWithDefaults)
{
    WriteTmpConf("");
    tsyncd::EngineOptions opt;
    const tsyncd::EngineOptions defaults;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.iface_name, defaults.iface_name);
    EXPECT_EQ(opt.ntp_port, defaults.ntp_port);
    EXPECT_NEAR(opt.ntp_offset_ewma_alpha, defaults.ntp_offset_ewma_alpha, 1e-9);
}

// ─── Comments and Blank Lines ────────────────────────────────

TEST_F(ConfigLoaderTest, HashComment_IsIgnored)
{
    WriteTmpConf("# this is a comment\niface_name = eth1\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.iface_name, "eth1");
}

TEST_F(ConfigLoaderTest, SemicolonComment_IsIgnored)
{
    WriteTmpConf("; another comment\nntp_port = 9000\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_port, 9000);
}

TEST_F(ConfigLoaderTest, BlankLines_AreIgnored)
{
    WriteTmpConf("\n\n   \n\nntp_port = 321\n\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_port, 321);
}

// ─── Section Headers ─────────────────────────────────────────

TEST_F(ConfigLoaderTest, SectionHeaders_AreAccepted_KeysStillParsed)
{
    WriteTmpConf("[gptp]\niface_name = enp3s0\n[ntp]\nntp_port = 555\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.iface_name, "enp3s0");
    EXPECT_EQ(opt.ntp_port, 555);
}

// ─── CRLF Line Endings ───────────────────────────────────────

TEST_F(ConfigLoaderTest, CrlfLineEndings_Parsed)
{
    WriteTmpConf("iface_name = eth2\r\nntp_port = 200\r\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.iface_name, "eth2");
    EXPECT_EQ(opt.ntp_port, 200);
}

// ─── String Keys ─────────────────────────────────────────────

TEST_F(ConfigLoaderTest, IfaceName_Parsed)
{
    WriteTmpConf("iface_name = enp3s0\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.iface_name, "enp3s0");
}

TEST_F(ConfigLoaderTest, PhcDevice_Parsed)
{
    WriteTmpConf("phc_device = /dev/ptp1\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.phc_device, "/dev/ptp1");
}

TEST_F(ConfigLoaderTest, ShmName_Parsed)
{
    WriteTmpConf("shm_name = /my_shm\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.shm_name, "/my_shm");
}

// ─── Integer Keys ────────────────────────────────────────────

TEST_F(ConfigLoaderTest, ShmSize_Parsed)
{
    WriteTmpConf("shm_size = 8192\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.shm_size, 8192u);
}

TEST_F(ConfigLoaderTest, NtpPort_Valid_Parsed)
{
    WriteTmpConf("ntp_port = 12345\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_port, 12345);
}

TEST_F(ConfigLoaderTest, NtpPort_Invalid_KeepsDefault)
{
    WriteTmpConf("ntp_port = notanumber\n");
    tsyncd::EngineOptions opt;
    const tsyncd::EngineOptions defaults;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_port, defaults.ntp_port);
}

TEST_F(ConfigLoaderTest, NtpQueryIntervalMs_Parsed)
{
    WriteTmpConf("ntp_query_interval_ms = 500\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_query_interval_ms, 500);
}

TEST_F(ConfigLoaderTest, NtpRequestTimeoutMs_Parsed)
{
    WriteTmpConf("ntp_request_timeout_ms = 100\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_request_timeout_ms, 100);
}

TEST_F(ConfigLoaderTest, NtpTimeoutMs_AliasWorks)
{
    WriteTmpConf("ntp_timeout_ms = 99\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_request_timeout_ms, 99);
}

TEST_F(ConfigLoaderTest, NtpSamplesToLock_Parsed)
{
    WriteTmpConf("ntp_samples_to_lock = 5\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_samples_to_lock, 5);
}

TEST_F(ConfigLoaderTest, UnstableOffsetThresholdNs_Parsed)
{
    WriteTmpConf("unstable_offset_threshold_ns = 50000\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.unstable_offset_threshold_ns, 50000);
}

TEST_F(ConfigLoaderTest, JumpFutureThresholdNs_Parsed)
{
    WriteTmpConf("jump_future_threshold_ns = 1000000000\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.jump_future_threshold_ns, 1'000'000'000LL);
}

// ─── Double Keys ─────────────────────────────────────────────

TEST_F(ConfigLoaderTest, NtpOffsetEwmaAlpha_Valid_Parsed)
{
    WriteTmpConf("ntp_offset_ewma_alpha = 0.5\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_NEAR(opt.ntp_offset_ewma_alpha, 0.5, 1e-9);
}

TEST_F(ConfigLoaderTest, NtpJitterEwmaAlpha_Valid_Parsed)
{
    WriteTmpConf("ntp_jitter_ewma_alpha = 0.1\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_NEAR(opt.ntp_jitter_ewma_alpha, 0.1, 1e-9);
}

TEST_F(ConfigLoaderTest, NtpOffsetEwmaAlpha_OutOfRange_KeepsDefault)
{
    WriteTmpConf("ntp_offset_ewma_alpha = 1.5\n");  // > 1.0 is invalid
    tsyncd::EngineOptions opt;
    const tsyncd::EngineOptions defaults;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_NEAR(opt.ntp_offset_ewma_alpha, defaults.ntp_offset_ewma_alpha, 1e-9);
}

// ─── abs_mode ────────────────────────────────────────────────

TEST_F(ConfigLoaderTest, AbsMode_PublishOnlyString_Parsed)
{
    WriteTmpConf("abs_mode = publish_only\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.abs_mode, tsyncd::EngineOptions::AbsMode::kPublishOnly);
}

TEST_F(ConfigLoaderTest, AbsMode_PublishOnlyHyphen_Parsed)
{
    WriteTmpConf("abs_mode = publish-only\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.abs_mode, tsyncd::EngineOptions::AbsMode::kPublishOnly);
}

TEST_F(ConfigLoaderTest, AbsMode_DisciplineString_Parsed)
{
    WriteTmpConf("abs_mode = discipline\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.abs_mode, tsyncd::EngineOptions::AbsMode::kDisciplineSystemClock);
}

TEST_F(ConfigLoaderTest, AbsMode_Numeric0_ParsedAsPublishOnly)
{
    WriteTmpConf("abs_mode = 0\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.abs_mode, tsyncd::EngineOptions::AbsMode::kPublishOnly);
}

TEST_F(ConfigLoaderTest, AbsMode_Numeric1_ParsedAsDiscipline)
{
    WriteTmpConf("abs_mode = 1\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.abs_mode, tsyncd::EngineOptions::AbsMode::kDisciplineSystemClock);
}

// ─── Boolean Keys ────────────────────────────────────────────

TEST_F(ConfigLoaderTest, AbsExternalEnable_TrueString_Parsed)
{
    WriteTmpConf("abs_external_enable = true\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_TRUE(opt.abs_external_enable);
}

TEST_F(ConfigLoaderTest, AbsExternalEnable_One_Parsed)
{
    WriteTmpConf("abs_external_enable = 1\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_TRUE(opt.abs_external_enable);
}

TEST_F(ConfigLoaderTest, AbsExternalEnable_FalseString_Parsed)
{
    WriteTmpConf("abs_external_enable = false\n");
    tsyncd::EngineOptions opt;
    opt.abs_external_enable = true;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_FALSE(opt.abs_external_enable);
}

TEST_F(ConfigLoaderTest, AbsExternalEnable_Zero_Parsed)
{
    WriteTmpConf("abs_external_enable = 0\n");
    tsyncd::EngineOptions opt;
    opt.abs_external_enable = true;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_FALSE(opt.abs_external_enable);
}

// ─── NTP Servers ─────────────────────────────────────────────

TEST_F(ConfigLoaderTest, NtpServers_Single_Parsed)
{
    WriteTmpConf("ntp_servers = time.google.com\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    ASSERT_EQ(opt.ntp_servers.size(), 1u);
    EXPECT_EQ(opt.ntp_servers[0], "time.google.com");
}

TEST_F(ConfigLoaderTest, NtpServers_MultipleViaComma_Parsed)
{
    WriteTmpConf("ntp_servers = 0.pool.ntp.org, 1.pool.ntp.org, 2.pool.ntp.org\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    ASSERT_EQ(opt.ntp_servers.size(), 3u);
    EXPECT_EQ(opt.ntp_servers[0], "0.pool.ntp.org");
    EXPECT_EQ(opt.ntp_servers[1], "1.pool.ntp.org");
    EXPECT_EQ(opt.ntp_servers[2], "2.pool.ntp.org");
}

// ─── Unknown Key ─────────────────────────────────────────────

TEST_F(ConfigLoaderTest, UnknownKey_IsIgnored_OtherKeysParsed)
{
    WriteTmpConf("unknown_key = some_value\nntp_port = 555\n");
    tsyncd::EngineOptions opt;
    ASSERT_TRUE(tsyncd::LoadEngineOptionsFromFile(kTmpConf, opt));
    EXPECT_EQ(opt.ntp_port, 555);
}

}  // namespace
