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

#include "score/json/json_parser.h"
#include "score/mw/log/logging.h"
#include "score/time_slave/src/common/logging_contexts.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace score
{
namespace ts
{
namespace
{

using JsonObject = score::json::Object;

namespace cs = config_schema;

/// Reads the properties of one JSON object and records every violation of the schema in
/// @c errors (it never stops at the first one, and never applies an invalid value).
///
/// Types are checked strictly: score::json::Any::As<arithmetic>() converts JSON booleans to
/// numbers and numbers to booleans, which the schema forbids, so the stored kind is checked first.
/// Finish() rejects keys the schema does not declare ("additionalProperties": false).
class JsonObjectReader
{
  public:
    template <std::size_t N>
    JsonObjectReader(const JsonObject& obj,
                     std::string_view scope,
                     const std::array<std::string_view, N>& schema_keys,
                     std::vector<std::string>& errors)
        : obj_{obj}, scope_{scope}, keys_begin_{schema_keys.begin()}, keys_end_{schema_keys.end()}, errors_{errors}
    {
    }

    /// Read a string property; leaves @p out unchanged if the key is missing or invalid.
    void String(const cs::StringProperty& prop, std::string& out)
    {
        const auto* value = Find(prop.key);
        if (value == nullptr)
        {
            return;
        }
        const auto result = value->As<std::string_view>();
        if (!result.has_value())
        {
            Error(prop.key, "has wrong type: expected string");
            return;
        }
        out = std::string{result.value()};
    }

    /// Read an integer property and validate it against the schema range; leaves @p out
    /// unchanged if the key is missing or invalid. The property is a template argument so that
    /// the compiler verifies the schema range fits into the destination type @p T.
    template <const cs::IntProperty& Prop, typename T>
    void Int(T& out)
    {
        static_assert(Prop.min >= std::numeric_limits<T>::min() && Prop.max <= std::numeric_limits<T>::max(),
                      "schema range of this property does not fit into the C++ field type");

        const auto* value = Find(Prop.key);
        if (value == nullptr)
        {
            return;
        }
        // Number::As<int64_t>() is a lossless cast: it rejects fractional and out-of-int64 values.
        const auto number = value->As<score::json::Number>();
        if (!number.has_value())
        {
            Error(Prop.key, "has wrong type: expected integer");
            return;
        }
        const auto result = number.value().get().As<std::int64_t>();
        if (!result.has_value())
        {
            Error(Prop.key, "has wrong type: expected integer");
            return;
        }
        if (result.value() < Prop.min || result.value() > Prop.max)
        {
            Error(Prop.key,
                  "is out of range: " + std::to_string(result.value()) + " not in [" + std::to_string(Prop.min) + ", " +
                      std::to_string(Prop.max) + "]");
            return;
        }
        out = static_cast<T>(result.value());
    }

    /// Read a bool property; leaves @p out unchanged if the key is missing or invalid.
    void Bool(const cs::BoolProperty& prop, bool& out)
    {
        const auto* value = Find(prop.key);
        if (value == nullptr)
        {
            return;
        }
        const auto result = value->As<bool>();
        if (value->As<score::json::Number>().has_value() || !result.has_value())
        {
            Error(prop.key, "has wrong type: expected boolean");
            return;
        }
        out = result.value();
    }

    /// Read a nested object property. Returns nullptr if the key is missing or not an object.
    const JsonObject* NestedObject(std::string_view key)
    {
        const auto* value = Find(key);
        if (value == nullptr)
        {
            return nullptr;
        }
        const auto result = value->As<JsonObject>();
        if (!result.has_value())
        {
            Error(key, "has wrong type: expected object");
            return nullptr;
        }
        return &result.value().get();
    }

    /// Report keys that the schema does not declare. Call after all properties were read.
    void Finish()
    {
        for (const auto& entry : obj_)
        {
            const auto key = entry.first.GetAsStringView();
            if (std::find(keys_begin_, keys_end_, key) == keys_end_)
            {
                Error(key, "is not defined by the configuration schema");
            }
        }
    }

  private:
    const score::json::Any* Find(std::string_view key) const
    {
        const auto it = obj_.find(key);
        return it == obj_.end() ? nullptr : &it->second;
    }

    void Error(std::string_view key, const std::string& reason)
    {
        std::string name{scope_};
        name += scope_.empty() ? "" : ".";
        name += key;
        errors_.push_back("Config field '" + name + "' " + reason);
    }

    const JsonObject& obj_;
    std::string_view scope_;
    const std::string_view* keys_begin_;
    const std::string_view* keys_end_;
    std::vector<std::string>& errors_;
};

/// Parse the optional "phc" nested object. If absent, @p cfg keeps defaults.
void ParsePhcConfig(JsonObjectReader& root, std::vector<std::string>& errors, details::PhcConfig& cfg)
{
    const JsonObject* phc_obj = root.NestedObject(cs::kPhcKey);
    if (phc_obj == nullptr)
    {
        return;
    }
    JsonObjectReader reader{*phc_obj, cs::kPhcKey, cs::kPhcKeys, errors};
    reader.Bool(cs::kPhcEnabled, cfg.enabled);
    reader.String(cs::kPhcDevice, cfg.device);
    reader.Int<cs::kPhcStepThresholdNs>(cfg.step_threshold_ns);
    reader.Finish();
}

/// Parse the optional "qnx" nested object. If absent, @p qnx keeps defaults.
void ParseQnxConfig(JsonObjectReader& root, std::vector<std::string>& errors, TimeSlaveConfig::QnxSettings& qnx)
{
    const JsonObject* qnx_obj = root.NestedObject(cs::kQnxKey);
    if (qnx_obj == nullptr)
    {
        return;
    }
    JsonObjectReader reader{*qnx_obj, cs::kQnxKey, cs::kQnxKeys, errors};
    reader.String(cs::kQnxBpfDevicePath, qnx.bpf_device_path);
    reader.Bool(cs::kQnxSeeSent, qnx.see_sent);
    reader.Finish();
}

}  // namespace

// req-Id: comp_req__time_slave__configuration
ConfigParseResult ParseConfig(const std::string& path)
{
    ConfigParseResult result;

    // A directory or a missing path is not a config file: keep the built-in defaults rather than
    // failing (see the directory-path crash fix). Invalid content is still rejected below.
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec))
    {
        score::mw::log::LogWarn(kTimeSlaveAppContext)
            << "Config path '" << path << "' is not a regular file, using built-in defaults";
        return result;
    }

    const score::json::JsonParser parser;
    const auto root_result = parser.FromFile(path);
    if (!root_result.has_value())
    {
        result.errors.push_back("Failed to parse config file '" + path +
                                "': " + std::string{root_result.error().Message()});
        return result;
    }

    const auto obj_result = root_result.value().As<JsonObject>();
    if (!obj_result.has_value())
    {
        result.errors.push_back("Config file '" + path + "' root is not a JSON object");
        return result;
    }

    auto& cfg = result.config;
    auto& opts = cfg.engine_opts;

    JsonObjectReader reader{obj_result.value().get(), "", cs::kRootKeys, result.errors};
    reader.String(cs::kIfaceName, opts.iface_name);
    reader.Int<cs::kDomainNumber>(opts.domain_number);
    reader.Int<cs::kPdelayReqIntervalMs>(opts.pdelay_req_interval_ms);
    reader.Int<cs::kPdelayWarmupMs>(opts.pdelay_warmup_ms);
    reader.Int<cs::kSyncTimeoutMs>(opts.sync_timeout_ms);
    reader.Int<cs::kJumpFutureThresholdNs>(opts.jump_future_threshold_ns);
    reader.String(cs::kShmPath, cfg.shm_path);

    ParsePhcConfig(reader, result.errors, opts.phc_config);
    ParseQnxConfig(reader, result.errors, cfg.qnx);
    reader.Finish();

    return result;
}

}  // namespace ts
}  // namespace score
