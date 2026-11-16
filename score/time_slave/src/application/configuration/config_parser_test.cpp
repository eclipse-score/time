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
#include "score/time_slave/src/application/configuration/config_parser.h"

#include "score/time_slave/src/application/configuration/config_schema_table.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace score
{
namespace ts
{
namespace
{

using ::testing::TestWithParam;
using ::testing::ValuesIn;

/// Write @p content to a temp file and return its path.
std::string WriteTempFile(const std::string& name, const std::string& content)
{
    const std::string path = testing::TempDir() + name;
    std::ofstream ofs(path);
    ofs << content;
    return path;
}

/// Parse @p path and require success.
TimeSlaveConfig ParseOk(const std::string& path)
{
    const auto result = ParseConfig(path);
    EXPECT_TRUE(result.ok()) << (result.errors.empty() ? "" : result.errors.front());
    return result.config;
}

TEST(ConfigParserTest, EmptyObjectReturnsDefaults)
{
    const auto path = WriteTempFile("empty_config.json", "{}");
    const auto cfg = ParseOk(path);

    // Parsing {} must leave every field at its struct default. (That those defaults match the
    // schema is checked by config_schema_consistency_test.)
    const TimeSlaveConfig defaults{};
    EXPECT_EQ(cfg.engine_opts.iface_name, defaults.engine_opts.iface_name);
    EXPECT_EQ(cfg.engine_opts.domain_number, defaults.engine_opts.domain_number);
    EXPECT_EQ(cfg.engine_opts.pdelay_req_interval_ms, defaults.engine_opts.pdelay_req_interval_ms);
    EXPECT_EQ(cfg.engine_opts.pdelay_warmup_ms, defaults.engine_opts.pdelay_warmup_ms);
    EXPECT_EQ(cfg.engine_opts.sync_timeout_ms, defaults.engine_opts.sync_timeout_ms);
    EXPECT_EQ(cfg.engine_opts.jump_future_threshold_ns, defaults.engine_opts.jump_future_threshold_ns);
    EXPECT_EQ(cfg.shm_path, defaults.shm_path);
    EXPECT_EQ(cfg.engine_opts.phc_config.enabled, defaults.engine_opts.phc_config.enabled);
    EXPECT_EQ(cfg.engine_opts.phc_config.device, defaults.engine_opts.phc_config.device);
    EXPECT_EQ(cfg.engine_opts.phc_config.step_threshold_ns, defaults.engine_opts.phc_config.step_threshold_ns);
    EXPECT_EQ(cfg.qnx.bpf_device_path, defaults.qnx.bpf_device_path);
    EXPECT_EQ(cfg.qnx.see_sent, defaults.qnx.see_sent);
}

TEST(ConfigParserTest, ParseAllTopLevelFields)
{
    const std::string json = R"({
        "iface_name": "eth1",
        "domain_number": 5,
        "pdelay_req_interval_ms": 500,
        "pdelay_warmup_ms": 1000,
        "sync_timeout_ms": 2000,
        "jump_future_threshold_ns": 100000000,
        "shm_path": "/custom_shm"
    })";
    const auto path = WriteTempFile("full_config.json", json);
    const auto cfg = ParseOk(path);

    EXPECT_EQ(cfg.engine_opts.iface_name, "eth1");
    EXPECT_EQ(cfg.engine_opts.domain_number, 5);
    EXPECT_EQ(cfg.engine_opts.pdelay_req_interval_ms, 500);
    EXPECT_EQ(cfg.engine_opts.pdelay_warmup_ms, 1000);
    EXPECT_EQ(cfg.engine_opts.sync_timeout_ms, 2000);
    EXPECT_EQ(cfg.engine_opts.jump_future_threshold_ns, 100'000'000LL);
    EXPECT_EQ(cfg.shm_path, "/custom_shm");
}

TEST(ConfigParserTest, ParsePhcConfig)
{
    const std::string json = R"({
        "phc": {
            "enabled": true,
            "device": "ptp0",
            "step_threshold_ns": 50000000
        }
    })";
    const auto path = WriteTempFile("phc_config.json", json);
    const auto cfg = ParseOk(path);

    EXPECT_TRUE(cfg.engine_opts.phc_config.enabled);
    EXPECT_EQ(cfg.engine_opts.phc_config.device, "ptp0");
    EXPECT_EQ(cfg.engine_opts.phc_config.step_threshold_ns, 50'000'000);
}

TEST(ConfigParserTest, ParseQnxConfig)
{
    const std::string json = R"({
        "qnx": {
            "bpf_device_path": "/dev/bpf_test",
            "see_sent": true
        }
    })";
    const auto path = WriteTempFile("qnx_config.json", json);
    const auto cfg = ParseOk(path);

    EXPECT_EQ(cfg.qnx.bpf_device_path, "/dev/bpf_test");
    EXPECT_TRUE(cfg.qnx.see_sent);
}

TEST(ConfigParserTest, ParsePartialFields)
{
    const std::string json = R"({
        "iface_name": "eth2"
    })";
    const auto path = WriteTempFile("partial_config.json", json);
    const auto cfg = ParseOk(path);

    // Only iface_name changed; everything else keeps default.
    EXPECT_EQ(cfg.engine_opts.iface_name, "eth2");
    EXPECT_EQ(cfg.engine_opts.domain_number, 0);
    EXPECT_EQ(cfg.engine_opts.pdelay_req_interval_ms, 1000);
    EXPECT_EQ(cfg.shm_path, "/gptp_shmem");
    EXPECT_FALSE(cfg.engine_opts.phc_config.enabled);
    EXPECT_FALSE(cfg.qnx.see_sent);
}

TEST(ConfigParserTest, FullExampleConfigRoundTrip)
{
    const auto path = WriteTempFile("example.json",
                                    "{\n"
                                    "    \"iface_name\": \"emac0\",\n"
                                    "    \"domain_number\": 0,\n"
                                    "    \"pdelay_req_interval_ms\": 1000,\n"
                                    "    \"pdelay_warmup_ms\": 2000,\n"
                                    "    \"sync_timeout_ms\": 3300,\n"
                                    "    \"jump_future_threshold_ns\": 500000000,\n"
                                    "    \"shm_path\": \"/gptp_shmem\",\n"
                                    "    \"phc\": {\n"
                                    "        \"enabled\": true,\n"
                                    "        \"device\": \"emac0\",\n"
                                    "        \"step_threshold_ns\": 100000000\n"
                                    "    },\n"
                                    "    \"qnx\": {\n"
                                    "        \"bpf_device_path\": \"/dev/bpf\",\n"
                                    "        \"see_sent\": true\n"
                                    "    }\n"
                                    "}\n");

    const auto cfg = ParseOk(path);

    EXPECT_EQ(cfg.engine_opts.iface_name, "emac0");
    EXPECT_EQ(cfg.engine_opts.domain_number, 0);
    EXPECT_EQ(cfg.engine_opts.pdelay_req_interval_ms, 1000);
    EXPECT_EQ(cfg.engine_opts.pdelay_warmup_ms, 2000);
    EXPECT_EQ(cfg.engine_opts.sync_timeout_ms, 3300);
    EXPECT_EQ(cfg.engine_opts.jump_future_threshold_ns, 500'000'000LL);
    EXPECT_EQ(cfg.shm_path, "/gptp_shmem");
    EXPECT_TRUE(cfg.engine_opts.phc_config.enabled);
    EXPECT_EQ(cfg.engine_opts.phc_config.device, "emac0");
    EXPECT_EQ(cfg.engine_opts.phc_config.step_threshold_ns, 100'000'000);
    EXPECT_EQ(cfg.qnx.bpf_device_path, "/dev/bpf");
    EXPECT_TRUE(cfg.qnx.see_sent);
}

// ---- Schema validation (issue #314) ----------------------------------------

/// Every key of the schema set to a non-default value, so a property the parser forgets to
/// read is caught by AllPropertiesAreApplied.
constexpr const char* kAllNonDefault = R"({
    "iface_name": "eth9",
    "domain_number": 7,
    "pdelay_req_interval_ms": 250,
    "pdelay_warmup_ms": 11,
    "sync_timeout_ms": 777,
    "jump_future_threshold_ns": 123456,
    "shm_path": "/other_shm",
    "phc": {"enabled": true, "device": "/dev/ptp7", "step_threshold_ns": 4242},
    "qnx": {"bpf_device_path": "/dev/bpf7", "see_sent": true}
})";

TEST(ConfigParserTest, AllPropertiesAreApplied)
{
    namespace cs = config_schema;
    const std::string json{kAllNonDefault};
    const auto expect_present = [&](const auto& keys) {
        for (const auto key : keys)
        {
            EXPECT_NE(json.find("\"" + std::string{key} + "\""), std::string::npos)
                << "add '" << key << "' to kAllNonDefault";
        }
    };
    expect_present(cs::kRootKeys);
    expect_present(cs::kPhcKeys);
    expect_present(cs::kQnxKeys);

    const auto cfg = ParseOk(WriteTempFile("all_non_default.json", kAllNonDefault));
    EXPECT_EQ(cfg.engine_opts.iface_name, "eth9");
    EXPECT_EQ(cfg.engine_opts.domain_number, 7);
    EXPECT_EQ(cfg.engine_opts.pdelay_req_interval_ms, 250);
    EXPECT_EQ(cfg.engine_opts.pdelay_warmup_ms, 11);
    EXPECT_EQ(cfg.engine_opts.sync_timeout_ms, 777);
    EXPECT_EQ(cfg.engine_opts.jump_future_threshold_ns, 123456);
    EXPECT_EQ(cfg.shm_path, "/other_shm");
    EXPECT_TRUE(cfg.engine_opts.phc_config.enabled);
    EXPECT_EQ(cfg.engine_opts.phc_config.device, "/dev/ptp7");
    EXPECT_EQ(cfg.engine_opts.phc_config.step_threshold_ns, 4242);
    EXPECT_EQ(cfg.qnx.bpf_device_path, "/dev/bpf7");
    EXPECT_TRUE(cfg.qnx.see_sent);
}

struct InvalidCase
{
    const char* name;
    const char* json;
    const char* expected_field;  // must appear in the error message
};

class ConfigParserInvalidTest : public TestWithParam<InvalidCase>
{
};

TEST_P(ConfigParserInvalidTest, RejectsValueViolatingSchema)
{
    const auto result =
        ParseConfig(WriteTempFile(std::string{"invalid_"} + GetParam().name + ".json", GetParam().json));
    ASSERT_FALSE(result.ok());
    ASSERT_EQ(result.errors.size(), 1U);
    EXPECT_NE(result.errors.front().find(GetParam().expected_field), std::string::npos) << result.errors.front();
}

const InvalidCase kInvalidCases[] = {
    {"domain_above_max", R"({"domain_number": 128})", "domain_number"},
    {"domain_negative", R"({"domain_number": -1})", "domain_number"},
    {"domain_wraps_uint8", R"({"domain_number": 256})", "domain_number"},
    {"pdelay_interval_below_min", R"({"pdelay_req_interval_ms": 9})", "pdelay_req_interval_ms"},
    {"pdelay_interval_above_max", R"({"pdelay_req_interval_ms": 60001})", "pdelay_req_interval_ms"},
    {"pdelay_warmup_negative", R"({"pdelay_warmup_ms": -1})", "pdelay_warmup_ms"},
    {"pdelay_warmup_above_max", R"({"pdelay_warmup_ms": 60001})", "pdelay_warmup_ms"},
    {"sync_timeout_below_min", R"({"sync_timeout_ms": 9})", "sync_timeout_ms"},
    {"sync_timeout_above_max", R"({"sync_timeout_ms": 60001})", "sync_timeout_ms"},
    {"jump_threshold_negative", R"({"jump_future_threshold_ns": -1})", "jump_future_threshold_ns"},
    {"step_threshold_negative", R"({"phc": {"step_threshold_ns": -1}})", "phc.step_threshold_ns"},
    {"bool_for_integer", R"({"domain_number": true})", "domain_number"},
    {"bool_false_for_integer", R"({"sync_timeout_ms": false})", "sync_timeout_ms"},
    {"bool_for_nested_integer", R"({"phc": {"step_threshold_ns": true}})", "phc.step_threshold_ns"},
    {"string_for_integer", R"({"domain_number": "5"})", "domain_number"},
    {"fractional_for_integer", R"({"domain_number": 1.5})", "domain_number"},
    {"null_for_integer", R"({"domain_number": null})", "domain_number"},
    {"integer_for_string", R"({"iface_name": 5})", "iface_name"},
    {"integer_for_bool", R"({"phc": {"enabled": 1}})", "phc.enabled"},
    {"string_for_bool", R"({"qnx": {"see_sent": "true"}})", "qnx.see_sent"},
    {"unknown_root_key", R"({"pdelay_interval_ms": 500})", "pdelay_interval_ms"},
    {"unknown_phc_key", R"({"phc": {"enable": true}})", "phc.enable"},
    {"unknown_qnx_key", R"({"qnx": {"see_send": true}})", "qnx.see_send"},
    {"phc_not_object", R"({"phc": 1})", "phc"},
    {"qnx_not_object", R"({"qnx": 1})", "qnx"},
    {"root_not_object", R"([])", "root is not a JSON object"},
    {"not_json", R"({"domain_number": )", "Failed to parse"},
};

INSTANTIATE_TEST_SUITE_P(Schema, ConfigParserInvalidTest, ValuesIn(kInvalidCases), [](const auto& info) {
    return std::string{info.param.name};
});

TEST(ConfigParserTest, ReportsAllErrorsNotJustTheFirst)
{
    const auto result = ParseConfig(WriteTempFile(
        "multi_error.json", R"({"domain_number": 128, "sync_timeout_ms": 1, "phc": {"enabled": 1}, "typo": 0})"));
    EXPECT_EQ(result.errors.size(), 4U);
}

TEST(ConfigParserTest, InvalidValueIsNeverApplied)
{
    const auto result = ParseConfig(WriteTempFile("not_applied.json", R"({"domain_number": 128})"));
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.config.engine_opts.domain_number, TimeSlaveConfig{}.engine_opts.domain_number);
}

/// Boundaries are taken from the table (which the consistency test ties to the schema).
TEST(ConfigParserTest, RangeBoundariesAreEnforcedExactly)
{
    namespace cs = config_schema;
    struct Field
    {
        const cs::IntProperty& prop;
        std::string prefix;  // JSON text before the value
        std::string suffix;  // JSON text after the value
    };
    const Field fields[] = {
        {cs::kDomainNumber, "{\"domain_number\": ", "}"},
        {cs::kPdelayReqIntervalMs, "{\"pdelay_req_interval_ms\": ", "}"},
        {cs::kPdelayWarmupMs, "{\"pdelay_warmup_ms\": ", "}"},
        {cs::kSyncTimeoutMs, "{\"sync_timeout_ms\": ", "}"},
        {cs::kJumpFutureThresholdNs, "{\"jump_future_threshold_ns\": ", "}"},
        {cs::kPhcStepThresholdNs, "{\"phc\": {\"step_threshold_ns\": ", "}}"},
    };
    for (const auto& f : fields)
    {
        const auto parse = [&](std::int64_t v) {
            return ParseConfig(WriteTempFile("range.json", f.prefix + std::to_string(v) + f.suffix));
        };
        EXPECT_TRUE(parse(f.prop.min).ok()) << f.prop.key;
        EXPECT_TRUE(parse(f.prop.max).ok()) << f.prop.key;
        if (f.prop.max != std::numeric_limits<std::int64_t>::max())
        {
            EXPECT_FALSE(parse(f.prop.max + 1).ok()) << f.prop.key;
        }
        if (f.prop.min != std::numeric_limits<std::int64_t>::min())
        {
            EXPECT_FALSE(parse(f.prop.min - 1).ok()) << f.prop.key;
        }
    }
}

TEST(ConfigParserTest, MissingFileIsReported)
{
    const auto result = ParseConfig(testing::TempDir() + "does_not_exist.json");
    ASSERT_FALSE(result.ok());
    EXPECT_NE(result.errors.front().find("does_not_exist.json"), std::string::npos);
}

}  // namespace
}  // namespace ts
}  // namespace score
