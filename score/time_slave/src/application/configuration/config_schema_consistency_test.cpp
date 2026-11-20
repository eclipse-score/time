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

// Guards the invariant "the parser enforces exactly what time_slave_config_schema.json says".
//
// The parser uses the hand-written table in config_schema_table.h. This test loads the real schema
// and compares it with the table in both directions (keys, types, ranges), checks the schema
// defaults against a default-constructed TimeSlaveConfig, and fails on any schema keyword the
// parser does not implement, so a schema change can never be silently ignored at runtime.

#include "score/time_slave/src/application/configuration/config_schema_table.h"
#include "score/time_slave/src/application/configuration/time_slave_config.h"

#include "score/json/json_parser.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <string_view>

namespace score
{
namespace ts
{
namespace
{

namespace cs = config_schema;
using JsonObject = score::json::Object;

/// Schema keywords the parser implements. Anything else (enum, pattern, exclusiveMinimum,
/// minLength, ...) would be silently ignored at runtime, so it must fail this test.
const std::set<std::string> kObjectKeywords = {"type", "additionalProperties", "required", "properties", "description"};
const std::set<std::string> kRootOnlyKeywords = {"$schema", "$id", "title"};
const std::set<std::string> kScalarKeywords = {"type", "minimum", "maximum", "default", "description"};

class SchemaConsistencyTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const char* path = std::getenv("TIME_SLAVE_CONFIG_SCHEMA");
        ASSERT_NE(path, nullptr) << "TIME_SLAVE_CONFIG_SCHEMA must point to the schema (see BUILD)";
        const score::json::JsonParser parser;
        auto parsed = parser.FromFile(path);
        ASSERT_TRUE(parsed.has_value()) << "cannot parse schema " << path;
        root_any_ = std::move(parsed).value();
        const auto root = root_any_.As<JsonObject>();
        ASSERT_TRUE(root.has_value());
        root_ = &root.value().get();
    }

    static std::set<std::string> Keys(const JsonObject& obj)
    {
        std::set<std::string> keys;
        for (const auto& entry : obj)
        {
            keys.emplace(entry.first.GetAsStringView());
        }
        return keys;
    }

    static const JsonObject& Child(const JsonObject& obj, std::string_view key)
    {
        const auto r = score::json::GetAttribute<JsonObject>(std::cref(obj), key);
        EXPECT_TRUE(r.has_value()) << "missing object '" << key << "'";
        static const JsonObject kEmpty;
        return r.has_value() ? r.value().get() : kEmpty;
    }

    static void ExpectKeywordsSubsetOf(const JsonObject& obj,
                                       const std::set<std::string>& allowed,
                                       const std::string& where,
                                       bool allow_root_only = false)
    {
        for (const auto& key : Keys(obj))
        {
            const bool ok = allowed.count(key) != 0 || (allow_root_only && kRootOnlyKeywords.count(key) != 0);
            EXPECT_TRUE(ok) << "schema keyword '" << key << "' at '" << where
                            << "' is not implemented by the config parser";
        }
    }

    static std::string Type(const JsonObject& obj)
    {
        const auto r = score::json::GetAttribute<std::string_view>(std::cref(obj), "type");
        return r.has_value() ? std::string{r.value()} : std::string{};
    }

    /// Check an object schema: closed set of properties, and exactly the table's keys.
    template <std::size_t N>
    static void ExpectObjectMatches(const JsonObject& schema,
                                    const std::array<std::string_view, N>& table_keys,
                                    const std::string& where,
                                    bool is_root)
    {
        ExpectKeywordsSubsetOf(schema, kObjectKeywords, where, is_root);
        EXPECT_EQ(Type(schema), "object") << where;
        const auto additional = score::json::GetAttribute<bool>(std::cref(schema), "additionalProperties");
        ASSERT_TRUE(additional.has_value()) << where << " must declare additionalProperties";
        EXPECT_FALSE(additional.value()) << where << ": the parser rejects unknown keys";
        if (schema.find("required") != schema.end())
        {
            const auto required = score::json::GetAttribute<score::json::List>(std::cref(schema), "required");
            ASSERT_TRUE(required.has_value());
            EXPECT_EQ(required.value().get().size(), 0U) << where << ": the parser does not support required keys";
        }

        const std::set<std::string> schema_keys = Keys(Child(schema, "properties"));
        std::set<std::string> table;
        for (const auto key : table_keys)
        {
            table.emplace(key);
        }
        EXPECT_EQ(schema_keys, table) << "schema properties and parser table differ at '" << where << "'";
    }

    static void ExpectInt(const JsonObject& props, const cs::IntProperty& p, std::int64_t struct_default)
    {
        const std::string key{p.key};
        const JsonObject& s = Child(props, key);
        ExpectKeywordsSubsetOf(s, kScalarKeywords, key);
        EXPECT_EQ(Type(s), "integer") << key;
        const auto min = score::json::GetAttribute<std::int64_t>(std::cref(s), "minimum");
        const auto max = score::json::GetAttribute<std::int64_t>(std::cref(s), "maximum");
        const auto def = score::json::GetAttribute<std::int64_t>(std::cref(s), "default");
        EXPECT_EQ(min.has_value() ? min.value() : std::numeric_limits<std::int64_t>::min(), p.min) << key;
        EXPECT_EQ(max.has_value() ? max.value() : std::numeric_limits<std::int64_t>::max(), p.max) << key;
        ASSERT_TRUE(def.has_value()) << key << " needs a default";
        EXPECT_EQ(def.value(), struct_default) << key << ": schema default differs from the C++ struct";
        EXPECT_GE(struct_default, p.min) << key << ": default outside range";
        EXPECT_LE(struct_default, p.max) << key << ": default outside range";
    }

    static void ExpectBool(const JsonObject& props, const cs::BoolProperty& p, bool struct_default)
    {
        const std::string key{p.key};
        const JsonObject& s = Child(props, key);
        ExpectKeywordsSubsetOf(s, kScalarKeywords, key);
        EXPECT_EQ(Type(s), "boolean") << key;
        const auto def = score::json::GetAttribute<bool>(std::cref(s), "default");
        ASSERT_TRUE(def.has_value()) << key << " needs a default";
        EXPECT_EQ(def.value(), struct_default) << key << ": schema default differs from the C++ struct";
    }

    static void ExpectString(const JsonObject& props, const cs::StringProperty& p, std::string_view struct_default)
    {
        const std::string key{p.key};
        const JsonObject& s = Child(props, key);
        ExpectKeywordsSubsetOf(s, kScalarKeywords, key);
        EXPECT_EQ(Type(s), "string") << key;
        const auto def = score::json::GetAttribute<std::string_view>(std::cref(s), "default");
        ASSERT_TRUE(def.has_value()) << key << " needs a default";
        EXPECT_EQ(def.value(), struct_default) << key << ": schema default differs from the C++ struct";
    }

    /// The C++ structs are the single C++ source of defaults; the schema must agree with them.
    const TimeSlaveConfig defaults_{};

    score::json::Any root_any_;
    const JsonObject* root_{nullptr};
};

TEST_F(SchemaConsistencyTest, RootMatchesTable)
{
    ExpectObjectMatches(*root_, cs::kRootKeys, "<root>", true);
    const JsonObject& props = Child(*root_, "properties");
    ExpectString(props, cs::kIfaceName, defaults_.engine_opts.iface_name);
    ExpectInt(props, cs::kDomainNumber, defaults_.engine_opts.domain_number);
    ExpectInt(props, cs::kPdelayReqIntervalMs, defaults_.engine_opts.pdelay_req_interval_ms);
    ExpectInt(props, cs::kPdelayWarmupMs, defaults_.engine_opts.pdelay_warmup_ms);
    ExpectInt(props, cs::kSyncTimeoutMs, defaults_.engine_opts.sync_timeout_ms);
    ExpectInt(props, cs::kJumpFutureThresholdNs, defaults_.engine_opts.jump_future_threshold_ns);
    ExpectString(props, cs::kShmPath, defaults_.shm_path);
}

TEST_F(SchemaConsistencyTest, PhcMatchesTable)
{
    const JsonObject& phc = Child(Child(*root_, "properties"), cs::kPhcKey);
    ExpectObjectMatches(phc, cs::kPhcKeys, "phc", false);
    const JsonObject& props = Child(phc, "properties");
    ExpectBool(props, cs::kPhcEnabled, defaults_.engine_opts.phc_config.enabled);
    ExpectString(props, cs::kPhcDevice, defaults_.engine_opts.phc_config.device);
    ExpectInt(props, cs::kPhcStepThresholdNs, defaults_.engine_opts.phc_config.step_threshold_ns);
}

TEST_F(SchemaConsistencyTest, QnxMatchesTable)
{
    const JsonObject& qnx = Child(Child(*root_, "properties"), cs::kQnxKey);
    ExpectObjectMatches(qnx, cs::kQnxKeys, "qnx", false);
    const JsonObject& props = Child(qnx, "properties");
    ExpectString(props, cs::kQnxBpfDevicePath, defaults_.qnx.bpf_device_path);
    ExpectBool(props, cs::kQnxSeeSent, defaults_.qnx.see_sent);
}

}  // namespace
}  // namespace ts
}  // namespace score
