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
#ifndef SCORE_TIME_SLAVE_CONFIGURATION_CONFIG_SCHEMA_TABLE_H
#define SCORE_TIME_SLAVE_CONFIGURATION_CONFIG_SCHEMA_TABLE_H

#include <array>
#include <cstdint>
#include <limits>
#include <string_view>

namespace score
{
namespace ts
{
namespace config_schema
{

/// Hand-written mirror of time_slave_config_schema.json (keys, types, ranges).
///
/// The parser enforces exactly these values. config_schema_consistency_test compares this
/// table with the schema file in both directions, so any drift fails the build's tests.
/// Defaults are not repeated here: they live in the C++ config structs, and the same test
/// checks those against the schema's "default" values.

struct IntProperty
{
    std::string_view key;
    std::int64_t min;
    std::int64_t max;
};

struct BoolProperty
{
    std::string_view key;
};

struct StringProperty
{
    std::string_view key;
};

inline constexpr std::int64_t kNoMax = std::numeric_limits<std::int64_t>::max();

// ---- root ----
inline constexpr StringProperty kIfaceName{"iface_name"};
inline constexpr IntProperty kDomainNumber{"domain_number", 0, 127};
inline constexpr IntProperty kPdelayReqIntervalMs{"pdelay_req_interval_ms", 10, 60000};
inline constexpr IntProperty kPdelayWarmupMs{"pdelay_warmup_ms", 0, 60000};
inline constexpr IntProperty kSyncTimeoutMs{"sync_timeout_ms", 10, 60000};
inline constexpr IntProperty kJumpFutureThresholdNs{"jump_future_threshold_ns", 0, kNoMax};
inline constexpr StringProperty kShmPath{"shm_path"};
inline constexpr std::string_view kPhcKey{"phc"};
inline constexpr std::string_view kQnxKey{"qnx"};
inline constexpr std::array kRootKeys{kIfaceName.key,
                                      kDomainNumber.key,
                                      kPdelayReqIntervalMs.key,
                                      kPdelayWarmupMs.key,
                                      kSyncTimeoutMs.key,
                                      kJumpFutureThresholdNs.key,
                                      kShmPath.key,
                                      kPhcKey,
                                      kQnxKey};

// ---- phc ----
inline constexpr BoolProperty kPhcEnabled{"enabled"};
inline constexpr StringProperty kPhcDevice{"device"};
inline constexpr IntProperty kPhcStepThresholdNs{"step_threshold_ns", 0, kNoMax};
inline constexpr std::array kPhcKeys{kPhcEnabled.key, kPhcDevice.key, kPhcStepThresholdNs.key};

// ---- qnx ----
inline constexpr StringProperty kQnxBpfDevicePath{"bpf_device_path"};
inline constexpr BoolProperty kQnxSeeSent{"see_sent"};
inline constexpr std::array kQnxKeys{kQnxBpfDevicePath.key, kQnxSeeSent.key};

}  // namespace config_schema
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_CONFIGURATION_CONFIG_SCHEMA_TABLE_H
