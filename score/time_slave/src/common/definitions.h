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

#ifndef SCORE_TIME_SLAVE_SRC_COMMON_DEFINITIONS_H
#define SCORE_TIME_SLAVE_SRC_COMMON_DEFINITIONS_H

#include <cstdlib>

namespace score::ts::env
{

/// Environment variable name for the TimeSlave configuration file path.
static constexpr auto kTimeSlaveConfigEnv = "TIMESLAVE_CONFIG";

/// Default path for the TimeSlave configuration file.
static constexpr auto kTimeSlaveConfigDefaultPath = "./etc/time_slave_config.json";

/// Environment variable name for the gPTP interface override.
static constexpr auto kGptpInterfaceEnv = "GPTP_IFACE";

static inline const char* GetEnvWithDefault(const char* env_var, const char* default_value)
{
    const char* value = std::getenv(env_var);
    return (value != nullptr && value[0] != '\0') ? value : default_value;
}

namespace qnx
{
/// Environment variable name for the QNX BPF clone-device path.
static constexpr auto kBpfDevicePathEnv = "QNX_BPF_DEVICE_PATH";

/// Default QNX BPF clone-device path.
static constexpr auto kBpfDevicePathDefault = "/dev/bpf";

/// Environment variable name for enabling raw SEE sent.
static constexpr auto kRawSeeSentEnv = "QNX_RAW_SEESENT";
}  // namespace qnx
}  // namespace score::ts::env

#endif  // SCORE_TIME_SLAVE_SRC_COMMON_DEFINITIONS_H
