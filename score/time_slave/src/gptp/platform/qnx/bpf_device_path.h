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

#ifndef SCORE_TIME_SLAVE_SRC_GPTP_PLATFORM_QNX_BPF_DEVICE_PATH_H
#define SCORE_TIME_SLAVE_SRC_GPTP_PLATFORM_QNX_BPF_DEVICE_PATH_H

#include "score/time_slave/src/common/definitions.h"

namespace score::ts::details
{

constexpr const char* ResolveBpfDevicePath(const char* configured_path) noexcept
{
    return (configured_path != nullptr && configured_path[0] != '\0') ? configured_path
                                                                      : env::qnx::kBpfDevicePathDefault;
}

}  // namespace score::ts::details

#endif  // SCORE_TIME_SLAVE_SRC_GPTP_PLATFORM_QNX_BPF_DEVICE_PATH_H
