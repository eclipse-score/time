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

namespace score::ts::env
{

/// Environment variable name for the TimeSlave configuration file path.
static constexpr auto kTimeSlaveConfigEnv = "TIMESLAVE_CONFIG";

/// Default path for the TimeSlave configuration file.
static constexpr auto kTimeSlaveConfigDefaultPath = "./etc/time_slave_config.json";

/// Environment variable name for the gPTP interface override.
static constexpr auto kGptpInterfaceEnv = "GPTP_IFACE";

namespace qnx
{
/// Environment variable name for the QNX BPF device prefix.
/// Used to override the default BPF device prefix (e.g., "/dev/bpf") for raw socket operations.
static constexpr auto kBpfDevicePrefixEnv = "QNX_BPF_DEVICE_PREFIX";

/// Default QNX BPF device prefix ("/dev/bpf").
static constexpr auto kBpfDevicePrefixDefault = "/dev/bpf";

/// Default QNX BPF device default("/dev/bpf0").
static constexpr auto kBpfDeviceDefault = "/dev/bpf0";

/// Environment variable name for enabling raw SEE sent.
static constexpr auto kRawSeeSentEnv = "QNX_RAW_SEESENT";
}  // namespace qnx
}  // namespace score::ts::env

#endif  // SCORE_TIME_SLAVE_SRC_COMMON_DEFINITIONS_H
